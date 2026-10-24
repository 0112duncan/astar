/*
 * This is intentionally dependency-free.  The course gateway speaks newline-
 * delimited TCP/JSON, while browsers only have WebSocket access.  This server
 * serves the static UI and translates between those two transports.
 */
const crypto = require('crypto');
const { spawn } = require('child_process');
const fs = require('fs');
const http = require('http');
const net = require('net');
const path = require('path');

const WEB_ROOT = __dirname;
const HTTP_PORT = Number(process.env.WEB_PORT || 8080);
const HUB_HOST = process.env.AUTOROB_HOST || '127.0.0.1';
const HUB_PORT = Number(process.env.AUTOROB_PORT || 9095);
const MIME_TYPES = {
  '.css': 'text/css; charset=utf-8',
  '.html': 'text/html; charset=utf-8',
  '.js': 'text/javascript; charset=utf-8',
  '.json': 'application/json; charset=utf-8',
};

function sendJson(socket, message) {
  if (!socket.destroyed) socket.write(`${JSON.stringify(message)}\n`);
}

// The existing C++ map node is deliberately a one-shot publisher. This
// service only launches that node; it never creates or publishes map data.
function startMapService() {
  let gateway;
  let buffer = '';

  function connect() {
    gateway = net.createConnection({ host: HUB_HOST, port: HUB_PORT });
    gateway.setNoDelay(true);
    gateway.on('connect', () => {
      console.log('Connected map-service launcher to Autorob gateway at ' + HUB_HOST + ':' + HUB_PORT);
      sendJson(gateway, { op: 'advertise_service', service: '/create_map', type: 'autorob/CreateMap' });
    });
    gateway.on('data', (chunk) => {
      buffer += chunk.toString('utf8');
      let newline;
      while ((newline = buffer.indexOf('\n')) !== -1) {
        const line = buffer.slice(0, newline);
        buffer = buffer.slice(newline + 1);
        if (!line.trim()) continue;
        try {
          const request = JSON.parse(line);
          if (request.op !== 'call_service' || request.service !== '/create_map') continue;
          let replied = false;
          const reply = (result, status = '') => {
            if (replied) return;
            replied = true;
            sendJson(gateway, {
              op: 'service_response', service: '/create_map', id: request.id,
              values: {}, result, status,
            });
          };
          const width = request.args?.width ?? 10;
          const height = request.args?.height ?? 10;
          if (!Number.isInteger(width) || !Number.isInteger(height) ||
              width < 1 || height < 1 || width > 100 || height > 100) {
            reply(false, 'width and height must be integers from 1 to 100');
            continue;
          }
          if (width * height < 2) {
            reply(false, 'map must contain at least two cells');
            continue;
          }
          const mapNode = spawn(path.join(WEB_ROOT, '..', 'build', 'map_node'), [String(width), String(height)], {
            cwd: path.join(WEB_ROOT, '..'),
            stdio: 'ignore',
          });
          mapNode.once('error', (error) => reply(false, 'could not start map_node: ' + error.message));
          mapNode.once('exit', (code) => reply(code === 0, code === 0 ? '' : 'map_node exited with code ' + code));
        } catch (error) {
          console.error('Ignoring invalid gateway message:', error.message);
        }
      }
    });
    gateway.on('error', (error) => console.error('Map-service gateway error: ' + error.message));
    gateway.on('close', () => {
      console.log('Map-service gateway connection closed; retrying in 1 second.');
      setTimeout(connect, 1000);
    });
  }
  connect();
}

function webSocketFrame(text) {
  const payload = Buffer.from(text);
  if (payload.length > 0xffff) throw new Error('WebSocket message is too large');
  if (payload.length < 126) return Buffer.concat([Buffer.from([0x81, payload.length]), payload]);
  const header = Buffer.alloc(4);
  header[0] = 0x81;
  header[1] = 126;
  header.writeUInt16BE(payload.length, 2);
  return Buffer.concat([header, payload]);
}

function attachWebSocket(socket) {
  let tcp;
  let tcpBuffer = '';
  let frameBuffer = Buffer.alloc(0);
  let closed = false;

  const close = () => {
    if (closed) return;
    closed = true;
    if (tcp) tcp.destroy();
    socket.destroy();
  };
  const sendText = (text) => {
    if (!closed && !socket.destroyed) socket.write(webSocketFrame(text));
  };

  tcp = net.createConnection({ host: HUB_HOST, port: HUB_PORT });
  tcp.setNoDelay(true);
  tcp.on('data', (chunk) => {
    tcpBuffer += chunk.toString('utf8');
    let newline;
    while ((newline = tcpBuffer.indexOf('\n')) !== -1) {
      const line = tcpBuffer.slice(0, newline);
      tcpBuffer = tcpBuffer.slice(newline + 1);
      if (line.trim()) sendText(line);
    }
  });
  tcp.on('error', (error) => sendText(JSON.stringify({ op: 'status', level: 'error', msg: `Gateway connection failed: ${error.message}` })));
  tcp.on('close', close);
  socket.on('error', close);
  socket.on('close', close);
  socket.on('data', (chunk) => {
    frameBuffer = Buffer.concat([frameBuffer, chunk]);
    while (frameBuffer.length >= 2) {
      const first = frameBuffer[0];
      const opcode = first & 0x0f;
      const masked = (frameBuffer[1] & 0x80) !== 0;
      let length = frameBuffer[1] & 0x7f;
      let headerLength = 2;
      if (length === 126) {
        if (frameBuffer.length < 4) return;
        length = frameBuffer.readUInt16BE(2);
        headerLength = 4;
      } else if (length === 127) {
        close();
        return;
      }
      const maskLength = masked ? 4 : 0;
      if (frameBuffer.length < headerLength + maskLength + length) return;
      const frameLength = headerLength + maskLength + length;
      const mask = frameBuffer.subarray(headerLength, headerLength + maskLength);
      const payload = Buffer.from(frameBuffer.subarray(headerLength + maskLength, frameLength));
      frameBuffer = frameBuffer.subarray(frameLength);
      if ((first & 0x80) === 0 || !masked) { close(); return; }
      if (opcode === 0x8) { close(); return; }
      if (opcode === 0x9) {
        socket.write(Buffer.concat([Buffer.from([0x8a, length]), payload]));
        continue;
      }
      if (opcode !== 0x1) { close(); return; }
      for (let i = 0; i < payload.length; i += 1) payload[i] ^= mask[i % 4];
      tcp.write(payload.toString('utf8') + '\n');
    }
  });
}

const server = http.createServer((request, response) => {
  const requestPath = new URL(request.url, `http://${request.headers.host || 'localhost'}`).pathname;
  const requested = requestPath === '/' ? '/index.html' : requestPath;
  const filePath = path.resolve(WEB_ROOT, `.${requested}`);
  if (!filePath.startsWith(`${WEB_ROOT}${path.sep}`)) {
    response.writeHead(403).end('Forbidden');
    return;
  }
  fs.readFile(filePath, (error, contents) => {
    if (error) {
      response.writeHead(error.code === 'ENOENT' ? 404 : 500).end('Not found');
      return;
    }
    response.writeHead(200, { 'Content-Type': MIME_TYPES[path.extname(filePath)] || 'application/octet-stream' });
    response.end(contents);
  });
});

server.on('upgrade', (request, socket) => {
  if (request.url !== '/ws' || request.headers.upgrade?.toLowerCase() !== 'websocket' || !request.headers['sec-websocket-key']) {
    socket.destroy();
    return;
  }
  const accept = crypto.createHash('sha1').update(`${request.headers['sec-websocket-key']}258EAFA5-E914-47DA-95CA-C5AB0DC85B11`).digest('base64');
  socket.write(`HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: ${accept}\r\n\r\n`);
  attachWebSocket(socket);
});

server.listen(HTTP_PORT, '127.0.0.1', () => console.log(`Open http://127.0.0.1:${HTTP_PORT}`));
startMapService();

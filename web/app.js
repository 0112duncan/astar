const canvas = document.querySelector('#map-canvas');
const context = canvas.getContext('2d');
const emptyState = document.querySelector('#empty-state');
const statusElement = document.querySelector('#status');
const detailsElement = document.querySelector('#map-details');
const connectionElement = document.querySelector('#connection');
const newMapButton = document.querySelector('#new-map');
const mapWidthInput = document.querySelector('#map-width');
const mapHeightInput = document.querySelector('#map-height');

let socket;
let map;
let start;
let goal;
let path = [];
let messageNumber = 0;
const pendingCalls = new Map();

function status(message, isError = false) {
  statusElement.textContent = message;
  statusElement.classList.toggle('error', isError);
}

function send(message) {
  if (!socket || socket.readyState !== WebSocket.OPEN) throw new Error('The bridge is not connected.');
  socket.send(JSON.stringify(message));
}

function callService(service, args) {
  const id = `web-call-${++messageNumber}`;
  send({ op: 'call_service', service, id, args });
  return new Promise((resolve, reject) => {
    const timer = window.setTimeout(() => {
      pendingCalls.delete(id);
      reject(new Error(`${service} timed out`));
    }, 10000);
    pendingCalls.set(id, { resolve, reject, timer });
  });
}

function isValidMap(candidate) {
  const info = candidate?.info;
  return Number.isInteger(info?.width) && Number.isInteger(info?.height) && info.width > 0 && info.height > 0 &&
    typeof info.resolution === 'number' && info.resolution > 0 && Array.isArray(candidate?.data) &&
    candidate.data.length === info.width * info.height;
}

function setMap(nextMap) {
  if (!isValidMap(nextMap)) {
    status('Received an invalid /map message.', true);
    return;
  }
  map = nextMap;
  start = undefined;
  goal = undefined;
  path = [];
  detailsElement.textContent = `${map.info.width} × ${map.info.height} cells · ${map.info.resolution} m/cell`;
  emptyState.hidden = true;
  draw();
  status('Map received. Choose a free start cell.');
}

function cellForWorld(position) {
  const origin = map.info.origin.position;
  return {
    x: Math.floor((position.x - origin.x) / map.info.resolution),
    y: Math.floor((position.y - origin.y) / map.info.resolution),
  };
}

function worldForCell(cell) {
  const origin = map.info.origin.position;
  return {
    x: origin.x + (cell.x + .5) * map.info.resolution,
    y: origin.y + (cell.y + .5) * map.info.resolution,
    z: 0,
  };
}

function poseForCell(cell) {
  return { header: { frame_id: 'map' }, pose: { position: worldForCell(cell), orientation: { x: 0, y: 0, z: 0, w: 1 } } };
}

function cellIsFree(cell) {
  return cell.x >= 0 && cell.y >= 0 && cell.x < map.info.width && cell.y < map.info.height &&
    map.data[cell.y * map.info.width + cell.x] >= 0 && map.data[cell.y * map.info.width + cell.x] < 50;
}

function canvasGeometry() {
  const longest = Math.max(map.info.width, map.info.height);
  const cellSize = Math.max(12, Math.min(42, Math.floor(780 / longest)));
  return { cellSize, width: map.info.width * cellSize, height: map.info.height * cellSize };
}

function pointForCell(cell, size) {
  return { x: (cell.x + .5) * size, y: (map.info.height - cell.y - .5) * size };
}

function drawMarker(cell, fill, label, size) {
  const point = pointForCell(cell, size);
  context.fillStyle = fill;
  context.beginPath();
  context.arc(point.x, point.y, size * .29, 0, Math.PI * 2);
  context.fill();
  context.fillStyle = '#ffffff';
  context.font = `700 ${Math.max(10, size * .32)}px system-ui`;
  context.textAlign = 'center';
  context.textBaseline = 'middle';
  context.fillText(label, point.x, point.y + 1);
}

function draw() {
  if (!map) return;
  const { cellSize, width, height } = canvasGeometry();
  const ratio = window.devicePixelRatio || 1;
  canvas.width = width * ratio;
  canvas.height = height * ratio;
  canvas.style.width = `${width}px`;
  canvas.style.height = `${height}px`;
  context.setTransform(ratio, 0, 0, ratio, 0, 0);
  context.clearRect(0, 0, width, height);

  for (let y = 0; y < map.info.height; y += 1) {
    for (let x = 0; x < map.info.width; x += 1) {
      const occupancy = map.data[y * map.info.width + x];
      context.fillStyle = occupancy >= 0 && occupancy < 50 ? '#dbeafe' : '#263047';
      context.fillRect(x * cellSize, (map.info.height - y - 1) * cellSize, cellSize, cellSize);
    }
  }
  context.strokeStyle = 'rgba(41, 55, 85, .3)';
  context.lineWidth = 1;
  for (let x = 0; x <= map.info.width; x += 1) { context.beginPath(); context.moveTo(x * cellSize, 0); context.lineTo(x * cellSize, height); context.stroke(); }
  for (let y = 0; y <= map.info.height; y += 1) { context.beginPath(); context.moveTo(0, y * cellSize); context.lineTo(width, y * cellSize); context.stroke(); }

  if (path.length) {
    context.strokeStyle = '#f59e0b';
    context.lineWidth = Math.max(3, cellSize * .24);
    context.lineCap = 'round';
    context.lineJoin = 'round';
    context.beginPath();
    path.forEach((cell, index) => {
      const point = pointForCell(cell, cellSize);
      if (index) context.lineTo(point.x, point.y); else context.moveTo(point.x, point.y);
    });
    context.stroke();
  }
  if (start) drawMarker(start, '#16a34a', 'S', cellSize);
  if (goal) drawMarker(goal, '#e11d48', 'G', cellSize);
}

async function plan() {
  status('Finding A* path…');
  try {
    const response = await callService('/plan_path', { start: poseForCell(start), goal: poseForCell(goal), tolerance: 0 });
    if (!response.result) throw new Error(response.status || 'No route was found.');
    const poses = response.values?.plan?.poses;
    if (!Array.isArray(poses)) throw new Error('Planner response has no path.');
    path = poses.map((pose) => cellForWorld(pose.pose.position));
    draw();
    status(`Route found: ${path.length} cell${path.length === 1 ? '' : 's'}. Click a free cell to start again.`);
  } catch (error) {
    path = [];
    draw();
    status(`Could not plan: ${error.message}`, true);
  }
}

canvas.addEventListener('click', (event) => {
  if (!map) return;
  const rectangle = canvas.getBoundingClientRect();
  const x = Math.floor((event.clientX - rectangle.left) * map.info.width / rectangle.width);
  const yFromTop = Math.floor((event.clientY - rectangle.top) * map.info.height / rectangle.height);
  const cell = { x, y: map.info.height - yFromTop - 1 };
  if (!cellIsFree(cell)) { status('That cell is blocked. Choose a free cell.', true); return; }
  if (!start || goal) {
    start = cell;
    goal = undefined;
    path = [];
    draw();
    status('Start set. Choose a free goal cell.');
  } else {
    goal = cell;
    draw();
    plan();
  }
});

newMapButton.addEventListener('click', async () => {
  const width = Number(mapWidthInput.value);
  const height = Number(mapHeightInput.value);
  if (!Number.isInteger(width) || !Number.isInteger(height) ||
      width < 1 || height < 1 || width > 100 || height > 100) {
    status('Width and height must be whole numbers from 1 to 100.', true);
    return;
  }
  if (width * height < 2) {
    status('The map must contain at least two cells.', true);
    return;
  }

  newMapButton.disabled = true;
  status('Creating and publishing a new map…');
  try {
    const response = await callService('/create_map', { width, height });
    if (!response.result) throw new Error(response.status || 'Map service failed.');
    // The /map publication normally arrives first, but using the returned map
    // also handles an unusually fast service response reliably.
    if (response.values?.map) setMap(response.values.map);
  } catch (error) {
    status(`Could not create map: ${error.message}`, true);
  } finally {
    newMapButton.disabled = false;
  }
});

function connect() {
  socket = new WebSocket(`ws://${window.location.host}/ws`);
  socket.addEventListener('open', () => {
    connectionElement.textContent = 'Gateway connected';
    connectionElement.classList.replace('offline', 'online');
    newMapButton.disabled = false;
    status('Connected. Press “New map” or wait for a /map publication.');
    send({ op: 'subscribe', topic: '/map', type: 'nav_msgs/OccupancyGrid' });
  });
  socket.addEventListener('message', ({ data }) => {
    let message;
    try { message = JSON.parse(data); } catch (_) { return; }
    if (message.op === 'publish' && message.topic === '/map') setMap(message.msg);
    if (message.op === 'service_response') {
      const pending = pendingCalls.get(message.id);
      if (!pending) return;
      window.clearTimeout(pending.timer);
      pendingCalls.delete(message.id);
      pending.resolve(message);
    }
    if (message.op === 'status' && message.level === 'error') status(message.msg, true);
  });
  socket.addEventListener('close', () => {
    connectionElement.textContent = 'Bridge disconnected';
    connectionElement.classList.replace('online', 'offline');
    newMapButton.disabled = true;
    status('Connection lost. Retrying…', true);
    window.setTimeout(connect, 1000);
  });
}

window.addEventListener('resize', draw);
connect();

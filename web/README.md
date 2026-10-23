# Autorob A* web interface

The course gateway uses TCP, so a browser needs the included small Node bridge.
It has no npm dependencies.

From the project root, start the planner runtime first:

```bash
make run
```

In another terminal, start this web application:

```bash
cd web
npm start
```

Open <http://127.0.0.1:8080>. The bridge subscribes the page to `/map` and
calls `/plan_path` after a start and goal are selected. It also provides the
`/create_map` service, which starts the existing one-shot C++ `map_node`. That
node generates and publishes the new OccupancyGrid on `/map`, then exits.

Optional environment variables: `WEB_PORT` (default `8080`), `AUTOROB_HOST`
(default `127.0.0.1`), and `AUTOROB_PORT` (default `9095`).

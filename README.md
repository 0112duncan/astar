# A* Map Planner

The A* Map Planner is an interactive occupancy-grid path planner. It generates
random maps, lets you choose start and goal cells in a browser, and uses A* to
find and display a collision-free route.

## Features

- Random occupancy-grid maps with configurable width and height
- Interactive start and goal selection
- Four-connected A* path planning around blocked cells
- Clear visualization of free cells, obstacles, and the planned route
- Automatic replanning when a new start and goal are selected
- Browser status messages for invalid selections and unreachable goals

## Requirements

- A C++17 compiler (such as `g++`)
- GNU Make
- Bash
- Node.js and npm

The web interface has no third-party npm dependencies, so no `npm install` step
is required.

## Start the planner

Open a terminal, change to the project directory, and start the runtime:

```bash
cd path/to/the/project
cd astar
make run
```

`make run` builds the project when necessary and starts the local TCP/JSON hub,
heap services, and A* planner on `127.0.0.1:9095`. Leave this terminal running.

Next, open a **new terminal** and start the web interface:

```bash
cd path/to/the/project
cd astar/web
npm start
```

Click the website address printed in the terminal, or open
[http://127.0.0.1:8080](http://127.0.0.1:8080) in a browser.

![A* planner finding a route through an occupancy grid](asset/astar_demo.gif)

## Plan a route

The browser interface creates a map and sends planning requests to the C++
runtime:

1. Enter a map **Width** and **Height** from 1 to 100.
2. Click **New map** to generate a random occupancy grid.
3. Click a free cell to set the start position.
4. Click another free cell to set the goal position.

The planner draws the resulting route in orange. The start is marked **S**, the
goal is marked **G**, and blocked cells are shown in dark blue. Click another
free cell to begin a new route.

If no route exists, the status message reports that planning failed. Generate a
new map or choose a different start and goal.

## How it works

The C++ runtime has A* planner, map generator, heap services, and local TCP/JSON hub with a ROS like pub sub system. The browser UI displays the map and route, then sends commands through a small local WebSocket-to-TCP bridge. Map generation and path planning remain in the C++ runtime.

# Autorob A* Map Planner

This project runs an A* path-planning service with a browser interface for creating occupancy-grid maps and selecting start and goal positions.

## Run the project

1. Open a terminal and change to the project folder:

   ```bash
   cd /path/to/astar
   ```

2. Build and start the planner runtime:

   ```bash
   make run
   ```

   Leave this terminal running.

3. Open a second terminal, return to the project folder, and enter the web folder:

   ```bash
   cd /path/to/astar
   cd web
   ```

4. Start the web interface:

   ```bash
   npm start
   ```

5. Open the address printed in the terminal in your web browser. By default, it is [http://127.0.0.1:8080](http://127.0.0.1:8080).

![A* planner demo](asset/astar_demo.gif)

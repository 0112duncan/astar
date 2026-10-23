# Simple ROS-like C++ Template

This version is intentionally written for learning. The TCP details live behind
`autorob::Node`; your assignment nodes only use four operations:

```cpp
node.publish("/topic", message);
node.subscribe("/topic", "type", callback);
node.advertise_service("/service", "type", callback);
auto reply = node.call_service("/service", arguments);
```

## What you should edit

| File | Your work |
| --- | --- |
| `src/nodes/min_heap_node.cpp` | Implement min-heap `/heapify` |
| `src/nodes/heap_sort_node.cpp` | Implement max-heap-based `/heap_sort` |
| `src/nodes/map_node.cpp` | Change the included valid map into your own map |
| `src/nodes/astar_node.cpp` | Validate maps and implement `/plan_path` and `/path` |

Start with `src/examples/demo_node.cpp`. It is a complete, short example of a
publisher, subscriber, and service provider.

You do not need to understand or modify `json.hpp`, `node.cpp`, or
`rosbridge_server.cpp` to complete the algorithms.

## Build and test the communication system

```bash
make build
make test
```

Expected result:

```text
PASS: topics, callbacks, services, and ID correlation work
```

The test starts the hub and demo node itself, so do not run `make run` at the
same time.

## Run the assignment processes

```bash
make run
```

This keeps the hub, min-heap service, heap-sort service, and A* node running.
In a second terminal:

```bash
make map
```

The autograder connects directly to the same hub at `127.0.0.1:9095` using the
protocol in `ROSBRIDGE_PROTOCOL.md`.

The communication test passes now. The Project 1 algorithm tests are expected
to fail until you complete the marked `TODO` sections.

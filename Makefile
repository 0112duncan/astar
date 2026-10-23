CXX ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Wpedantic -pthread
CPPFLAGS ?= -Iinclude

NODE_CLIENT = src/middleware/node.cpp
HEADERS = $(wildcard include/*.hpp)

.PHONY: build run map test clean

build: build/autorob_hub build/min_heap_node build/heap_sort_node build/astar_node build/map_node

build/autorob_hub: src/hub.cpp src/rosbridge_server.cpp src/middleware/bus.cpp $(HEADERS)
	mkdir -p build
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) src/hub.cpp src/rosbridge_server.cpp \
		src/middleware/bus.cpp -o $@

build/min_heap_node: src/nodes/min_heap_node.cpp $(NODE_CLIENT) $(HEADERS)
	mkdir -p build
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) src/nodes/min_heap_node.cpp $(NODE_CLIENT) -o $@

build/heap_sort_node: src/nodes/heap_sort_node.cpp $(NODE_CLIENT) $(HEADERS)
	mkdir -p build
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) src/nodes/heap_sort_node.cpp $(NODE_CLIENT) -o $@

build/astar_node: src/nodes/astar_node.cpp $(NODE_CLIENT) $(HEADERS)
	mkdir -p build
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) src/nodes/astar_node.cpp $(NODE_CLIENT) -o $@

build/map_node: src/nodes/map_node.cpp $(NODE_CLIENT) $(HEADERS)
	mkdir -p build
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) src/nodes/map_node.cpp $(NODE_CLIENT) -o $@

build/ping_node: src/nodes/ping_node.cpp $(NODE_CLIENT) $(HEADERS)
	mkdir -p build
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) src/nodes/ping_node.cpp $(NODE_CLIENT) -o $@

build/pong_node: src/nodes/pong_node.cpp $(NODE_CLIENT) $(HEADERS)
	mkdir -p build
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) src/nodes/pong_node.cpp $(NODE_CLIENT) -o $@

build/demo_node: src/examples/demo_node.cpp $(NODE_CLIENT) $(HEADERS)
	mkdir -p build
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) src/examples/demo_node.cpp $(NODE_CLIENT) -o $@

run: build
	./scripts/run.sh

map: build/map_node
	./build/map_node

test: build/autorob_hub build/demo_node
	python3 tests/test_gateway.py

clean:
	rm -rf build

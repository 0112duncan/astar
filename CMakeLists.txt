cmake_minimum_required(VERSION 3.16)
project(simple_autorob LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
find_package(Threads REQUIRED)

add_library(autorob_node src/middleware/node.cpp)
target_include_directories(autorob_node PUBLIC include)
target_link_libraries(autorob_node PUBLIC Threads::Threads)

add_executable(autorob_hub src/hub.cpp src/rosbridge_server.cpp src/middleware/bus.cpp)
target_include_directories(autorob_hub PRIVATE include)
target_link_libraries(autorob_hub PRIVATE Threads::Threads)

foreach(target min_heap_node heap_sort_node astar_node map_node ping_node pong_node)
  add_executable(${target} src/nodes/${target}.cpp)
  target_link_libraries(${target} PRIVATE autorob_node)
endforeach()

add_executable(demo_node src/examples/demo_node.cpp)
target_link_libraries(demo_node PRIVATE autorob_node)

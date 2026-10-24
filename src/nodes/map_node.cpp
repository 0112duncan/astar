#include "node.hpp"
#include <vector>
#include <algorithm>
#include <stdexcept>
#include <cmath>
#include <iostream>
#include <string>

namespace {

int parse_dimension(const char* text, const char* name) {
  std::size_t parsed = 0;
  int value = std::stoi(text, &parsed);
  if (text[parsed] != '\0' || value < 1 || value > 100) {
    throw std::invalid_argument(std::string(name) + " must be an integer from 1 to 100");
  }
  return value;
}

}  // namespace

//function to create random map
autorob::Json create_map(
  int width,
  int height,
  double resolution,
  double origin_x,
  double origin_y
){
  if (width <= 0 || height <= 0 || width * height < 2) {
      throw std::invalid_argument("map too small");
  }

  if (resolution <= 0) {
      throw std::invalid_argument("resolution must be positive");
  }

  std::vector<int> map_arr(width * height, 25);
  int random_obstacle_amount = height*width/50;
  //put the random obstacle spot and grow the obstacle
  std::srand(std::time(nullptr));
  for (int i=0; i<random_obstacle_amount; i++){
    int spot = (std::rand()%(height*width));

    int max_size = std::max(1, std::min(width, height) / 4);
    int size = 1 + std::rand() % max_size;

    int cx = spot%width;
    int cy = spot/width;
    for (int y = cy - size/2; y <= cy + size/2; y++) {
      for (int x = cx - size/2; x <= cx + size/2; x++) {
        // map edge check
        if (x >= width || x < 0 || y >= height || y < 0 ) {
          continue;
        }

        // diamond
        if (std::abs(x - cx) + std::abs(y - cy) <= size/2) {
            map_arr[y * width + x] = 100;
        }
      }
    }
  }

  //check 4connected freecell
  map_arr[0] = 25;

  if (width >= 2) {
      map_arr[1] = 25;
  } else {
      map_arr[width] = 25;   
  }

  autorob::Json::Array data;
  for (int value : map_arr) {
      data.push_back(autorob::Json(value));
  }

  //put generated map in Json format
  autorob::Json map = autorob::Json::Object{
      {"header", autorob::Json::Object{{"frame_id", "map"}}},
      {"info",
      autorob::Json::Object{
          {"resolution", resolution},
          {"width", width},
          {"height", height},
          {"origin",
            autorob::Json::Object{
                {"position", autorob::Json::Object{{"x", origin_x}, {"y", origin_y}, {"z", 0.0}}},
                {"orientation",
                autorob::Json::Object{{"x", 0.0}, {"y", 0.0},
                                      {"z", 0.0}, {"w", 1.0}}}}}}},
      {"data", data}};
  return map;
}

int main(int argc, char* argv[]) {
  try {
    if (argc != 1 && argc != 3) {
      throw std::invalid_argument("usage: map_node [width height]");
    }
    const int width = argc == 3 ? parse_dimension(argv[1], "width") : 10;
    const int height = argc == 3 ? parse_dimension(argv[2], "height") : 10;

    autorob::Node node("map_node");
    node.advertise("/map", "nav_msgs/OccupancyGrid");

    node.publish("/map", create_map(width, height, 0.5, -1.0, 2.0));
    return 0;  // map_node publishes once and exits.
  } catch (const std::exception& error) {
    std::cerr << "map_node: " << error.what() << '\n';
    return 1;
  }
}
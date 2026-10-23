#include "node.hpp"
#include <vector>
#include <algorithm>
#include <stdexcept>
#include <cmath>

//function to create random map
autorob::Json create_map(
  int width,
  int height,
  double resolution,
  double origin_x,
  double origin_y
){
  if (width * height < 2) {
      throw std::invalid_argument("map too small");
  }

  if (resolution <= 0) {
      throw std::invalid_argument("resolution must be positive");
  }

  std::vector<int> map_arr(width * height, 25);
  int random_obstacle_amount = height*width/20;
  //put the random obstacle spot and grow the obstacle
  std::srand(std::time(nullptr));
  for (int i=0; i<random_obstacle_amount; i++){
    int spot = (std::rand()%(height*width));

    int max_size = std::max(1, std::min(width, height) / 2);
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

int main() {
  autorob::Node node("map_node");
  node.advertise("/map", "nav_msgs/OccupancyGrid");
  
  node.publish("/map", create_map(10, 10, 0.5, -1.0, 2.0));
  return 0;  // make map publishes once and exits.
}
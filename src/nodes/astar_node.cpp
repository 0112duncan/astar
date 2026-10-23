#include "node.hpp"

#include <mutex>
#include <iostream>
#include <algorithm>
#include <cmath>

int main() {
  autorob::Node node("astar_node");
  std::mutex map_mutex;
  autorob::Json current_map;  // null means no map has arrived yet.

  // Subscriber template: save each newer /map message.
  node.subscribe("/map", "nav_msgs/OccupancyGrid",
                 [&map_mutex, &current_map](const autorob::Json& message) {
                   std::lock_guard<std::mutex> lock(map_mutex);
                   current_map = message;
                 });

  node.advertise("/path", "nav_msgs/Path");

  // Service-server template for /plan_path.
  node.advertise_service(
    "/plan_path", "autorob/PlanPath",
    [&node, &map_mutex, &current_map](const autorob::Json& args) {
      autorob::Json map_copy;
      {
        std::lock_guard<std::mutex> lock(map_mutex);
        map_copy = current_map;
      }
      if (map_copy.is_null()) {
        return autorob::ServiceResult{false, autorob::Json::Object{},
                                      "no map received yet"};
      }

      // TODO 1: Read args.get("start"), args.get("goal"), and tolerance.
      const autorob::Json* start = args.get("start");
      const autorob::Json* goal = args.get("goal");
      const autorob::Json* tolerance = args.get("tolerance");
      // TODO 2: Convert world coordinates to grid cells.
      double start_world_x = start->get("pose")->get("position")->get("x")->as_number();
      double start_world_y = start->get("pose")->get("position")->get("y")->as_number();
      double goal_world_x = goal->get("pose")->get("position")->get("x")->as_number();
      double goal_world_y = goal->get("pose")->get("position")->get("y")->as_number();
      double resolution = map_copy.get("info")->get("resolution")->as_number();
      double origin_x = map_copy.get("info")->get("origin")->get("position")->get("x")->as_number();
      double origin_y = map_copy.get("info")->get("origin")->get("position")->get("y")->as_number();

      int start_x = std::floor((start_world_x - origin_x)/resolution);
      int start_y = std::floor((start_world_y - origin_y)/resolution);
      int goal_x = std::floor((goal_world_x - origin_x)/resolution);
      int goal_y = std::floor((goal_world_y - origin_y)/resolution);
      // TODO 3: Reject out-of-bounds or blocked cells.
      int width = static_cast<int>(map_copy.get("info")->get("width")->as_number());
      int height = static_cast<int>(map_copy.get("info")->get("height")->as_number());

      const autorob::Json::Array& data = map_copy.get("data")->as_array();
      std::vector<double> obstacle(data.size());
      for (int i=0; i<data.size(); i++){
        obstacle[i] = data[i].as_number();
      }

      if (start_x >= width || start_x < 0 || start_y >= height || start_y < 0) {
        std::cout << "start out of bounds\n";   // optional

        return autorob::ServiceResult{
            false,
            autorob::Json::Object{},
            "start out of bounds"
        };
      }

      if (obstacle[start_y*width + start_x] >= 50 || obstacle[start_y*width + start_x] < 0) {
        std::cout << "start blocked\n";   // optional

        return autorob::ServiceResult{
            false,
            autorob::Json::Object{},
            "start blocked"
        };
      }

      if (goal_x >= width || goal_x < 0 || goal_y >= height || goal_y < 0) {
        std::cout << "goal out of bounds\n";

        return autorob::ServiceResult{
            false,
            autorob::Json::Object{},
            "goal out of bounds"
        };
      }

      if (obstacle[goal_y*width + goal_x] >= 50 || obstacle[goal_y*width + goal_x] < 0) {
        std::cout << "goal blocked\n";   // optional

        return autorob::ServiceResult{
            false,
            autorob::Json::Object{},
            "goal blocked"
        };
      }

      // TODO 4: Run four-connected A* and store parents.
      std::vector<int> g_score(width * height, 99999);
      g_score[start_y * width + start_x] = 0;
      std::vector<int> f_score(width * height, 99999);
      f_score[start_y * width + start_x] = std::abs(start_x - goal_x) + std::abs(start_y - goal_y);

      std::vector<int>discovered_node;
      std::vector<int>searched_node = {start_y*width + start_x};
      std::vector<int> came_from(width * height, -1);
      int node_now = start_y*width + start_x;

      while (node_now != goal_y*width + goal_x){
        //node now right node
        if (node_now%width + 1 < width && obstacle[node_now + 1] < 50 && obstacle[node_now + 1] >= 0 
          && std::find(searched_node.begin(),searched_node.end(),node_now + 1) == searched_node.end()){
          if (g_score[node_now + 1] > g_score[node_now] + 1) {
            g_score[node_now + 1] = g_score[node_now] + 1;
            came_from[node_now + 1] = node_now;
          }
          int h = std::abs((node_now + 1)%width - goal_x) + std::abs((node_now + 1)/width - goal_y);
          f_score[node_now + 1] = g_score[node_now + 1] + h;
          if (std::find(discovered_node.begin(), discovered_node.end(), node_now + 1) == discovered_node.end()) {
            discovered_node.push_back(node_now + 1);
          }
        }
        //node now left node
        if (node_now%width > 0 && obstacle[node_now - 1] < 50 && obstacle[node_now - 1] >= 0 
          && std::find(searched_node.begin(),searched_node.end(),node_now - 1) == searched_node.end()){
          if (g_score[node_now - 1] > g_score[node_now] + 1) {
            g_score[node_now - 1] = g_score[node_now] + 1;
            came_from[node_now - 1] = node_now;
          }
          int h = std::abs((node_now - 1)%width - goal_x) + std::abs((node_now - 1)/width - goal_y);
          f_score[node_now - 1] = g_score[node_now - 1] + h;
          if (std::find(discovered_node.begin(), discovered_node.end(), node_now - 1) == discovered_node.end()) {
            discovered_node.push_back(node_now - 1);
          }
        }
        //node now up node
        if (node_now/width > 0 && obstacle[node_now - width] < 50 && obstacle[node_now - width] >= 0 
          && std::find(searched_node.begin(),searched_node.end(),node_now - width) == searched_node.end()){
          if (g_score[node_now - width] > g_score[node_now] + 1) {
            g_score[node_now - width] = g_score[node_now] + 1;
            came_from[node_now - width] = node_now;
          }
          int h = std::abs((node_now - width)%width - goal_x) + std::abs((node_now - width)/width - goal_y);
          f_score[node_now - width] = g_score[node_now - width] + h;
          if (std::find(discovered_node.begin(), discovered_node.end(), node_now - width) == discovered_node.end()) {
            discovered_node.push_back(node_now - width);
          }
        }
        //node now down node
        if (node_now/width + 1 < height && obstacle[node_now + width] < 50 && obstacle[node_now + width] >= 0 
          && std::find(searched_node.begin(),searched_node.end(),node_now + width) == searched_node.end()){
          if (g_score[node_now + width] > g_score[node_now] + 1) {
            g_score[node_now + width] = g_score[node_now] + 1;
            came_from[node_now + width] = node_now;
          }
          int h = std::abs((node_now + width)%width - goal_x) + std::abs((node_now + width)/width - goal_y);
          f_score[node_now + width] = g_score[node_now + width] + h;
          if (std::find(discovered_node.begin(), discovered_node.end(), node_now + width) == discovered_node.end()) {
            discovered_node.push_back(node_now + width);
          }
        }
        //compare and put best one in searched node
        if (discovered_node.empty()) {
          return autorob::ServiceResult{
            false,
            autorob::Json::Object{},
            "no path found"
          };
        }

        autorob::Json::Array priorities;

        for (int index : discovered_node) {
          priorities.push_back(autorob::Json(f_score[index]));
        }

        auto sorted = node.call_service(
          "/heap_sort",
          autorob::Json::Object{
            {"numbers", autorob::Json(priorities)}
          }
        );

        if (!sorted.result) {
          return sorted;
        }

        const autorob::Json::Array& sorted_array = sorted.values.get("sorted")->as_array();
        int smallest_f = static_cast<int>(sorted_array[0].as_number());
        int best_pos = 0;

        for (int i = 0; i < discovered_node.size(); i++) {
          if (f_score[discovered_node[i]] == smallest_f) {
            best_pos = i;
            break;
          }
        }

        node_now = discovered_node[best_pos];
        discovered_node.erase(discovered_node.begin() + best_pos);
        searched_node.push_back(node_now);
      }

      // TODO 5: reconstruct path from came_from
      std::vector<int> path;
      int current = goal_y*width + goal_x;

      while (current != start_y*width + start_x) {
        path.push_back(current);
        current = came_from[current];

        if (current == -1) {
          return autorob::ServiceResult{
            false,
            autorob::Json::Object{},
            "failed to reconstruct path"
          };
        }
      }

      path.push_back(start_y*width + start_x);

      std::reverse(path.begin(), path.end());

      autorob::Json::Array poses;

      for (int index : path) {
        int cell_x = index % width;
        int cell_y = index / width;

        double world_x = origin_x + (cell_x + 0.5) * resolution;
        double world_y = origin_y + (cell_y + 0.5) * resolution;

        autorob::Json pose = autorob::Json::Object{
          {"header", autorob::Json::Object{
            {"frame_id", "map"}
          }},
          {"pose", autorob::Json::Object{
            {"position", autorob::Json::Object{
              {"x", world_x},
              {"y", world_y},
              {"z", 0.0}
            }},
            {"orientation", autorob::Json::Object{
              {"x", 0.0},
              {"y", 0.0},
              {"z", 0.0},
              {"w", 1.0}
            }}
          }}
        };

        poses.push_back(pose);
      }

      autorob::Json plan = autorob::Json::Object{
        {"header", autorob::Json::Object{
            {"frame_id", "map"}
        }},
        {"poses", autorob::Json(poses)}
      };

      node.publish("/path", plan);

      return autorob::ServiceResult{
        true,
        autorob::Json::Object{
          {"plan", plan}
        },
        ""
      };
    });
  node.wait();
}

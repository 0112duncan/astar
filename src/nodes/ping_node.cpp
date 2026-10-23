#include "node.hpp"

#include <chrono>
#include <iostream>
#include <thread>

int main() {
  autorob::Node node("ping_node");
  node.advertise("/ping", "std_msgs/String");

  node.subscribe("/pong", "std_msgs/String", [&node](const autorob::Json& message) {
    std::cout << "received /pong: " << message.dump() << '\n';
  });

  // Publish at a fixed rate so the exchange also works when pong starts later.
  for (;;) {
    node.publish("/ping", autorob::Json::Object{{"data", "ping"}});
    std::cout << "published /ping" << '\n';
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }
}

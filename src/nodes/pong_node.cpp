#include "node.hpp"

#include <iostream>

int main() {
  autorob::Node node("pong_node");
  node.advertise("/pong", "std_msgs/String");

  node.subscribe("/ping", "std_msgs/String", [&node](const autorob::Json& message) {
    std::cout << "received /ping: " << message.dump() << '\n';
    node.publish("/pong", autorob::Json::Object{{"data", "pong"}});
    std::cout << "published /pong" << '\n';
  });

  node.wait();
}

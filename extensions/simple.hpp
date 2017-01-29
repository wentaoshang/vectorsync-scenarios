/* -*- Mode:C++; c-file-style:"google"; indent-tabs-mode:nil; -*- */

#ifndef SIMPLE_HPP_
#define SIMPLE_HPP_

#include <functional>
#include <random>

#include "vsync.hpp"

namespace ndn {
namespace vsync {
namespace app {

class SimpleNode {
 public:
  SimpleNode(const NodeID& nid, const Name& prefix, KeyChain& keychain)
      : scheduler_(face_.getIoService()),
        key_chain_(keychain),
        node_(face_, scheduler_, key_chain_, nid, prefix,
              std::bind(&SimpleNode::OnData, this, _1, _2, _3)),
        rengine_(rdevice_()),
        rdist_(500, 10000) {}

  void Start() {
    scheduler_.scheduleEvent(time::milliseconds(rdist_(rengine_)),
                             [this] { PublishData(); });
    face_.processEvents();
  }

 private:
  void OnData(const std::string& content, const ViewID& vi,
              const VersionVector& vv) {
    std::cout << "Upcall OnData: content=\"" << content << "\", vi=" << vi
              << ", vv=" << vv << std::endl;
  }

  void PublishData() {
    node_.PublishData("Hello from " + node_.GetNodeID());
    scheduler_.scheduleEvent(time::milliseconds(rdist_(rengine_)),
                             [this] { PublishData(); });
  }

  Face face_;
  Scheduler scheduler_;
  KeyChain& key_chain_;
  Node node_;

  std::random_device rdevice_;
  std::mt19937 rengine_;
  std::uniform_int_distribution<> rdist_;
};

// int main(int argc, char* argv[]) {
//   // Create a simple view with three nodes
//   if (argc != 3) {
//     std::cerr << "Usage: " << argv[0] << " [node_id] [node_prefix]"
//               << std::endl;
//     return -1;
//   }

//   NodeID nid = argv[1];
//   Name prefix(argv[2]);

//   SimpleNode node(nid, prefix);
//   node.Start();
//   return 0;
// }

}  // namespace app
}  // namespace vsync
}  // namespace ndn

#endif  // SIMPLE_HPP_

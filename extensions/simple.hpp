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
  using OnDataEventTraceCb = std::function<void(
      const std::string&, const ViewID&, const VersionVector&)>;

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

  void ConnectVectorClockTrace(Node::VectorClockChangeCb cb) {
    node_.ConnectVectorClockChangeSignal(cb);
  }

  void ConnectViewIDTrace(Node::ViewIDChangeCb cb) {
    node_.ConnectViewIDChangeSignal(cb);
  }

  void ConnectOnDataEventTrace(OnDataEventTraceCb cb) {
    on_data_event_trace_.connect(cb);
  }

 private:
  void OnData(const std::string& content, const ViewID& vi,
              const VersionVector& vv) {
    on_data_event_trace_(content, vi, vv);
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

  util::Signal<SimpleNode, const std::string&, const ViewID&,
               const VersionVector&>
      on_data_event_trace_;
};

}  // namespace app
}  // namespace vsync
}  // namespace ndn

#endif  // SIMPLE_HPP_

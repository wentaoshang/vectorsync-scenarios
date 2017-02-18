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
  // First parameter is the new Data; second parameter indicates whether the
  // data is published locally.
  using DataEventTraceCb =
      std::function<void(std::shared_ptr<const Data>, bool)>;

  SimpleNode(const NodeID& nid, const Name& prefix, KeyChain& keychain,
             uint32_t seed)
      : scheduler_(face_.getIoService()),
        key_chain_(keychain),
        node_(face_, scheduler_, key_chain_, nid, prefix,
              std::bind(&SimpleNode::OnData, this, _1, _2, _3, _4)),
        rengine_(seed),
        rdist_(500, 10000) {}

  void Start() {
    // Wait for 8 seconds before publishing the first data packet.
    // This allows the view change process to stablize.
    scheduler_.scheduleEvent(time::milliseconds(8000 + rdist_(rengine_)),
                             [this] { PublishData(); });
    face_.processEvents();
  }

  void ConnectVectorClockTrace(Node::VectorClockChangeCb cb) {
    node_.ConnectVectorClockChangeSignal(cb);
  }

  void ConnectViewIDTrace(Node::ViewIDChangeCb cb) {
    node_.ConnectViewIDChangeSignal(cb);
  }

  void ConnectDataEventTrace(DataEventTraceCb cb) {
    data_event_trace_.connect(cb);
  }

 private:
  void OnData(std::shared_ptr<const Data> data, const std::string& content,
              const ViewID& vi, const VersionVector& vv) {
    data_event_trace_(data, false);
  }

  void PublishData() {
    std::shared_ptr<const Data> data;
    std::tie(data, std::ignore, std::ignore) =
        node_.PublishData("Hello from " + node_.GetNodeID());
    data_event_trace_(data, true);
    scheduler_.scheduleEvent(time::milliseconds(rdist_(rengine_)),
                             [this] { PublishData(); });
  }

  Face face_;
  Scheduler scheduler_;
  KeyChain& key_chain_;
  Node node_;

  std::mt19937 rengine_;
  std::uniform_int_distribution<> rdist_;

  util::Signal<SimpleNode, std::shared_ptr<const Data>, bool> data_event_trace_;
};

}  // namespace app
}  // namespace vsync
}  // namespace ndn

#endif  // SIMPLE_HPP_

/* -*- Mode:C++; c-file-style:"google"; indent-tabs-mode:nil; -*- */

#ifndef SIMPLE_FIFO_HPP_
#define SIMPLE_FIFO_HPP_

#include <functional>
#include <random>

#include "fifo.hpp"

namespace ndn {
namespace vsync {
namespace app {

class SimpleFIFONode {
 public:
  // First parameter is the new Data; second parameter indicates whether the
  // data is published locally.
  using DataEventTraceCb =
      std::function<void(std::shared_ptr<const Data>, bool)>;

  SimpleFIFONode(const Name& nid, KeyChain& keychain, uint32_t seed)
      : scheduler_(face_.getIoService()),
        key_chain_(keychain),
        node_(face_, scheduler_, key_chain_, nid, seed),
        rengine_(seed),
        rdist_(500, 10000) {
    node_.ConnectFIFODataSignal(std::bind(&SimpleFIFONode::OnData, this, _1));
  }

  void Start() {
    // Wait for 8 seconds before publishing the first data packet.
    // This allows the view change process to stablize.
    scheduler_.scheduleEvent(time::milliseconds(8000 + rdist_(rengine_)),
                             [this] { PublishData(); });
    face_.processEvents();
  }

  void ConnectVectorChangeTrace(Node::VectorChangeCb cb) {
    node_.ConnectVectorChangeSignal(cb);
  }

  void ConnectViewChangeTrace(Node::ViewChangeCb cb) {
    node_.ConnectViewChangeSignal(cb);
  }

  void ConnectDataEventTrace(DataEventTraceCb cb) {
    data_event_trace_.connect(cb);
  }

 private:
  void OnData(std::shared_ptr<const Data> data) {
    data_event_trace_(data, false);
  }

  void PublishData() {
    auto data =
        node_.PublishFIFOData("Hello from " + node_.GetNodeID().toUri());
    data_event_trace_(data, true);
    scheduler_.scheduleEvent(time::milliseconds(rdist_(rengine_)),
                             [this] { PublishData(); });
  }

  Face face_;
  Scheduler scheduler_;
  KeyChain& key_chain_;
  FIFONode node_;

  std::mt19937 rengine_;
  std::uniform_int_distribution<> rdist_;

  util::Signal<SimpleFIFONode, std::shared_ptr<const Data>, bool>
      data_event_trace_;
};

}  // namespace app
}  // namespace vsync
}  // namespace ndn

#endif  // SIMPLE_FIFO_HPP_

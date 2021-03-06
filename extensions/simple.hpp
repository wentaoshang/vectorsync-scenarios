/* -*- Mode:C++; c-file-style:"google"; indent-tabs-mode:nil; -*- */

#ifndef SIMPLE_HPP_
#define SIMPLE_HPP_

#include <functional>
#include <random>
#include <stdexcept>

#include "node.hpp"

namespace ndn {
namespace vsync {
namespace app {

class SimpleNode {
 public:
  // First parameter is the new Data; second parameter indicates whether the
  // data is published locally.
  using DataEventTraceCb =
      std::function<void(std::shared_ptr<const Data>, bool)>;

  SimpleNode(const Name& nid, KeyChain& keychain, uint32_t seed,
             double data_rate)
      : scheduler_(face_.getIoService()),
        key_chain_(keychain),
        node_(face_, scheduler_, key_chain_, nid, seed),
        rengine_(seed),
        rdist_(data_rate) {
    node_.ConnectDataSignal(std::bind(&SimpleNode::OnData, this, _1));
  }

  void SetViewInfo(const ViewInfo& vinfo) {
    if (vinfo.Size() == 0) return;
    auto nid = node_.GetNodeID();
    if (!vinfo.GetIndexByID(nid).second)
      throw std::invalid_argument("Cannot load ViewInfo at node " +
                                  nid.toUri());

    auto leader = vinfo.GetIDByIndex(vinfo.Size() - 1).first;
    node_.SetViewInfo({1, leader}, vinfo);
  }

  void Start() {
    node_.Start();
    scheduler_.scheduleEvent(
        time::milliseconds(static_cast<int>(1000.0 * rdist_(rengine_))),
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
    if (++data_count_ > 100) return;
    std::string msg =
        node_.GetNodeID().toUri() + ":" + std::to_string(data_count_);
    auto data = node_.PublishData(msg);
    data_event_trace_(data, true);
    scheduler_.scheduleEvent(
        time::milliseconds(static_cast<int>(1000.0 * rdist_(rengine_))),
        [this] { PublishData(); });
  }

  Face face_;
  Scheduler scheduler_;
  KeyChain& key_chain_;
  Node node_;
  int data_count_ = 0;

  std::mt19937 rengine_;
  std::exponential_distribution<> rdist_;

  util::Signal<SimpleNode, std::shared_ptr<const Data>, bool> data_event_trace_;
};

}  // namespace app
}  // namespace vsync
}  // namespace ndn

#endif  // SIMPLE_HPP_

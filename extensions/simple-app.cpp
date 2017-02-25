/* -*- Mode:C++; c-file-style:"google"; indent-tabs-mode:nil; -*- */

#include "simple-app.hpp"

NS_LOG_COMPONENT_DEFINE("ns3.ndn.vsync.SimpleNodeApp");

namespace ns3 {
namespace ndn {
namespace vsync {

NS_OBJECT_ENSURE_REGISTERED(SimpleNodeApp);

void SimpleNodeApp::StartApplication() {
  NS_LOG_INFO("NodeID: " << node_id_ << " Seed: " << seed_);
  node_.reset(new ::ndn::vsync::app::SimpleNode(
      node_id_, routing_prefix_, ndn::StackHelper::getKeyChain(), seed_));
  node_->ConnectVectorClockTrace(
      std::bind(&SimpleNodeApp::TraceVectorClock, this, _1, _2));
  node_->ConnectViewChangeTrace(
      std::bind(&SimpleNodeApp::TraceViewChange, this, _1, _2, _3));
  node_->ConnectDataEventTrace(
      std::bind(&SimpleNodeApp::TraceDataEvent, this, _1, _2));
  node_->Start();
}

}  // namespace vsync
}  // namespace ndn
}  // namespace ns3

/* -*- Mode:C++; c-file-style:"google"; indent-tabs-mode:nil; -*- */

#include "simple-causal-app.hpp"

NS_LOG_COMPONENT_DEFINE("ns3.ndn.vsync.SimpleCOApp");

namespace ns3 {
namespace ndn {
namespace vsync {

NS_OBJECT_ENSURE_REGISTERED(SimpleCOApp);

void SimpleCOApp::StartApplication() {
  NS_LOG_INFO("NodeID: " << node_id_ << " Seed: " << seed_);
  node_.reset(new ::ndn::vsync::app::SimpleCONode(
      node_id_, ndn::StackHelper::getKeyChain(), seed_));
  node_->ConnectVectorChangeTrace(
      std::bind(&SimpleCOApp::TraceVectorChange, this, _1, _2));
  node_->ConnectViewChangeTrace(
      std::bind(&SimpleCOApp::TraceViewChange, this, _1, _2, _3));
  node_->ConnectDataEventTrace(
      std::bind(&SimpleCOApp::TraceDataEvent, this, _1, _2));
  node_->Start();
}

}  // namespace vsync
}  // namespace ndn
}  // namespace ns3

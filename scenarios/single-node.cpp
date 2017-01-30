/* -*- Mode:C++; c-file-style:"google"; indent-tabs-mode:nil; -*- */

#include "simple-app.hpp"

#include "ns3/core-module.h"
#include "ns3/ndnSIM-module.h"
#include "ns3/network-module.h"

NS_LOG_COMPONENT_DEFINE("ns3.ndn.vsync.scenarios.SingleNode");

namespace ns3 {

static void VectorClockChange(::ndn::vsync::VersionVector vc) {
  NS_LOG_LOGIC(Simulator::Now().GetSeconds() << " vector_clock=" << vc);
}

int main(int argc, char* argv[]) {
  LogComponentEnable("ns3.ndn.vsync.scenarios.SingleNode", LOG_LEVEL_LOGIC);

  CommandLine cmd;
  cmd.Parse(argc, argv);

  Ptr<Node> node = CreateObject<Node>();

  ndn::StackHelper ndnHelper;
  ndnHelper.Install(node);

  ndn::AppHelper appHelper("ns3::ndn::vsync::SimpleNodeApp");
  appHelper.SetAttribute("NodeID", StringValue("B"));
  appHelper.Install(node).Start(Seconds(1.5));

  node->GetApplication(0)->TraceConnectWithoutContext(
      "VectorClock", MakeCallback(&VectorClockChange));

  Simulator::Stop(Seconds(60.0));

  Simulator::Run();
  Simulator::Destroy();

  return 0;
}

}  // namespace ns3

int main(int argc, char* argv[]) { return ns3::main(argc, argv); }

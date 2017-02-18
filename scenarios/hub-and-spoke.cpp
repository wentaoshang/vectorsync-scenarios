/* -*- Mode:C++; c-file-style:"google"; indent-tabs-mode:nil; -*- */

#include "simple-app.hpp"

#include "ns3/core-module.h"
#include "ns3/ndnSIM-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/random-variable-stream.h"

NS_LOG_COMPONENT_DEFINE("ns3.ndn.vsync.scenarios.HubAndSpoke");

namespace ns3 {

static void VectorClockChange(const ::ndn::vsync::VersionVector& vc) {
  NS_LOG_INFO("vector_clock=" << vc);
}

static void ViewIDChange(const ::ndn::vsync::ViewID& vid) {
  NS_LOG_INFO("view_id=" << vid);
}

int main(int argc, char* argv[]) {
  Config::SetDefault("ns3::PointToPointNetDevice::DataRate",
                     StringValue("10Mbps"));
  Config::SetDefault("ns3::PointToPointChannel::Delay", StringValue("1ms"));
  Config::SetDefault("ns3::DropTailQueue::MaxPackets", StringValue("100"));

  CommandLine cmd;
  cmd.Parse(argc, argv);

  NodeContainer nodes;
  nodes.Create(4);

  // Node 0 is central hub
  PointToPointHelper p2p;
  p2p.Install(nodes.Get(0), nodes.Get(1));
  p2p.Install(nodes.Get(0), nodes.Get(2));
  p2p.Install(nodes.Get(0), nodes.Get(3));

  ndn::StackHelper ndnHelper;
  ndnHelper.SetDefaultRoutes(true);
  ndnHelper.InstallAll();

  ndn::StrategyChoiceHelper::InstallAll(::ndn::vsync::kSyncPrefix.toUri(),
                                        "/localhost/nfd/strategy/multicast");

  Ptr<UniformRandomVariable> x = CreateObject<UniformRandomVariable>();

  ndn::AppHelper helper1("ns3::ndn::vsync::SimpleNodeApp");
  helper1.SetAttribute("NodeID", StringValue("N1"));
  helper1.SetAttribute("RandomSeed", UintegerValue(x->GetInteger()));
  helper1.Install(nodes.Get(1)).Start(Seconds(1.0));

  ndn::AppHelper helper2("ns3::ndn::vsync::SimpleNodeApp");
  helper2.SetAttribute("NodeID", StringValue("N2"));
  helper2.SetAttribute("RandomSeed", UintegerValue(x->GetInteger()));
  helper2.SetAttribute("StartTime", TimeValue(Seconds(10.0)));
  helper2.SetAttribute("StopTime", TimeValue(Seconds(35.0)));
  helper2.Install(nodes.Get(2));

  ndn::AppHelper helper3("ns3::ndn::vsync::SimpleNodeApp");
  helper3.SetAttribute("NodeID", StringValue("N3"));
  helper3.SetAttribute("RandomSeed", UintegerValue(x->GetInteger()));
  helper3.Install(nodes.Get(3)).Start(Seconds(20.0));

  for (int i = 1; i <= 3; ++i) {
    ndn::FibHelper::AddRoute(nodes.Get(0), ::ndn::vsync::kSyncPrefix.toUri(),
                             nodes.Get(i), 1);
    StringValue nid;
    nodes.Get(i)->GetApplication(0)->GetAttribute("NodeID", nid);
    std::string node_prefix = "/" + nid.Get();
    ndn::FibHelper::AddRoute(nodes.Get(0), node_prefix, nodes.Get(i), 1);

    ndn::FibHelper::AddRoute(nodes.Get(i), "/", nodes.Get(0), 1);
    ndn::FibHelper::AddRoute(nodes.Get(i), ::ndn::vsync::kSyncPrefix.toUri(),
                             nodes.Get(0), 1);

    nodes.Get(i)->GetApplication(0)->TraceConnectWithoutContext(
        "VectorClock", MakeCallback(&VectorClockChange));
    nodes.Get(i)->GetApplication(0)->TraceConnectWithoutContext(
        "ViewID", MakeCallback(&ViewIDChange));
  }

  Simulator::Stop(Seconds(60.0));

  Simulator::Run();
  Simulator::Destroy();

  return 0;
}

}  // namespace ns3

int main(int argc, char* argv[]) { return ns3::main(argc, argv); }

/* -*- Mode:C++; c-file-style:"google"; indent-tabs-mode:nil; -*- */

#include "simple-app.hpp"

#include "ns3/core-module.h"
#include "ns3/ndnSIM-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/random-variable-stream.h"

NS_LOG_COMPONENT_DEFINE("ns3.ndn.vsync.scenarios.Line");

namespace ns3 {

int main(int argc, char* argv[]) {
  Config::SetDefault("ns3::PointToPointNetDevice::DataRate",
                     StringValue("10Mbps"));
  Config::SetDefault("ns3::PointToPointChannel::Delay", StringValue("1ms"));
  Config::SetDefault("ns3::DropTailQueue::MaxPackets", StringValue("100"));

  CommandLine cmd;
  cmd.Parse(argc, argv);

  NodeContainer nodes;
  nodes.Create(4);

  PointToPointHelper p2p;
  p2p.Install(nodes.Get(0), nodes.Get(1));
  p2p.Install(nodes.Get(1), nodes.Get(2));
  p2p.Install(nodes.Get(2), nodes.Get(3));

  ndn::StackHelper ndnHelper;
  ndnHelper.SetDefaultRoutes(true);
  ndnHelper.InstallAll();

  ndn::StrategyChoiceHelper::InstallAll(::ndn::vsync::kSyncPrefix.toUri(),
                                        "/localhost/nfd/strategy/multicast");

  Ptr<UniformRandomVariable> x = CreateObject<UniformRandomVariable>();

  ndn::AppHelper helper0("ns3::ndn::vsync::SimpleNodeApp");
  helper0.SetAttribute("NodeID", StringValue("N0"));
  helper0.SetAttribute("RandomSeed", UintegerValue(x->GetInteger()));
  helper0.Install(nodes.Get(0)).Start(Seconds(1.0));

  ndn::AppHelper helper1("ns3::ndn::vsync::SimpleNodeApp");
  helper1.SetAttribute("NodeID", StringValue("N1"));
  helper1.SetAttribute("RandomSeed", UintegerValue(x->GetInteger()));
  helper1.Install(nodes.Get(1)).Start(Seconds(1.0));

  ndn::AppHelper helper2("ns3::ndn::vsync::SimpleNodeApp");
  helper2.SetAttribute("NodeID", StringValue("N2"));
  helper2.SetAttribute("RandomSeed", UintegerValue(x->GetInteger()));
  helper2.Install(nodes.Get(2)).Start(Seconds(20.0));

  ndn::AppHelper helper3("ns3::ndn::vsync::SimpleNodeApp");
  helper3.SetAttribute("NodeID", StringValue("N3"));
  helper3.SetAttribute("RandomSeed", UintegerValue(x->GetInteger()));
  helper3.Install(nodes.Get(3)).Start(Seconds(1.0));

  ndn::FibHelper::AddRoute(nodes.Get(0), ::ndn::vsync::kSyncPrefix.toUri(),
                           nodes.Get(1), 1);
  ndn::FibHelper::AddRoute(nodes.Get(0), "/", nodes.Get(1), 1);

  ndn::FibHelper::AddRoute(nodes.Get(1), ::ndn::vsync::kSyncPrefix.toUri(),
                           nodes.Get(0), 1);
  ndn::FibHelper::AddRoute(nodes.Get(1), "/N0", nodes.Get(0), 1);
  ndn::FibHelper::AddRoute(nodes.Get(1), ::ndn::vsync::kSyncPrefix.toUri(),
                           nodes.Get(2), 1);
  ndn::FibHelper::AddRoute(nodes.Get(1), "/", nodes.Get(2), 1);

  ndn::FibHelper::AddRoute(nodes.Get(2), ::ndn::vsync::kSyncPrefix.toUri(),
                           nodes.Get(1), 1);
  ndn::FibHelper::AddRoute(nodes.Get(2), "/", nodes.Get(1), 1);
  ndn::FibHelper::AddRoute(nodes.Get(2), ::ndn::vsync::kSyncPrefix.toUri(),
                           nodes.Get(3), 1);
  ndn::FibHelper::AddRoute(nodes.Get(2), "/N3", nodes.Get(3), 1);

  ndn::FibHelper::AddRoute(nodes.Get(3), ::ndn::vsync::kSyncPrefix.toUri(),
                           nodes.Get(2), 1);
  ndn::FibHelper::AddRoute(nodes.Get(3), "/", nodes.Get(2), 1);

  Simulator::Stop(Seconds(60.0));

  Simulator::Run();
  Simulator::Destroy();

  return 0;
}

}  // namespace ns3

int main(int argc, char* argv[]) { return ns3::main(argc, argv); }

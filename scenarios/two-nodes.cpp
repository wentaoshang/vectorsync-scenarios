/* -*- Mode:C++; c-file-style:"google"; indent-tabs-mode:nil; -*- */

#include "simple-app.hpp"
#include "simple.hpp"

#include "ns3/core-module.h"
#include "ns3/ndnSIM-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"

namespace ns3 {

int main(int argc, char* argv[]) {
  Config::SetDefault("ns3::PointToPointNetDevice::DataRate",
                     StringValue("10Mbps"));
  Config::SetDefault("ns3::PointToPointChannel::Delay", StringValue("1ms"));
  Config::SetDefault("ns3::DropTailQueue::MaxPackets", StringValue("100"));

  CommandLine cmd;
  cmd.Parse(argc, argv);

  NodeContainer nodes;
  nodes.Create(2);

  PointToPointHelper p2p;
  p2p.Install(nodes.Get(0), nodes.Get(1));

  ndn::StackHelper ndnHelper;
  ndnHelper.SetDefaultRoutes(true);
  ndnHelper.InstallAll();

  ndn::StrategyChoiceHelper::InstallAll("/vsync",
                                        "/localhost/nfd/strategy/multicast");

  ndn::AppHelper helper0("ns3::ndn::vsync::SimpleNodeApp");
  helper0.SetAttribute("NodeID", StringValue("A"));
  helper0.Install(nodes.Get(0)).Start(Seconds(1.0));

  ndn::AppHelper helper1("ns3::ndn::vsync::SimpleNodeApp");
  helper1.SetAttribute("NodeID", StringValue("B"));
  helper1.Install(nodes.Get(1)).Start(Seconds(1.0));

  ndn::FibHelper::AddRoute(nodes.Get(0), "/vsync", nodes.Get(1), 1);
  ndn::FibHelper::AddRoute(nodes.Get(1), "/vsync", nodes.Get(0), 1);

  Simulator::Stop(Seconds(60.0));

  Simulator::Run();
  Simulator::Destroy();

  return 0;
}

}  // namespace ns3

int main(int argc, char* argv[]) { return ns3::main(argc, argv); }

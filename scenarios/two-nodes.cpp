/* -*- Mode:C++; c-file-style:"google"; indent-tabs-mode:nil; -*- */

#include "simple-app.hpp"

#include <algorithm>
#include <iostream>
#include <string>
#include <unordered_map>

#include "ns3/core-module.h"
#include "ns3/ndnSIM-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/random-variable-stream.h"

NS_LOG_COMPONENT_DEFINE("ns3.ndn.vsync.scenarios.TwoNodes");

namespace ns3 {

std::unordered_map<std::string, std::pair<double, double>> delays;

static void DataEvent(std::shared_ptr<const ndn::Data> data, bool is_local) {
  auto name = data->getName().toUri();
  NS_LOG_INFO("new data: name=" << name << ", isLocal="
                                << (is_local ? "true" : "false"));
  double now = Simulator::Now().GetSeconds();
  auto& entry = delays[name];
  if (is_local)
    entry.first = now;
  else
    entry.second = now;
}

int main(int argc, char* argv[]) {
  Config::SetDefault("ns3::PointToPointNetDevice::DataRate",
                     StringValue("10Mbps"));
  Config::SetDefault("ns3::DropTailQueue::MaxPackets", StringValue("100"));

  double TotalRunTimeSeconds = 3600.0;
  bool Synchronized = false;
  double LossRate = 0.0;
  std::string LinkDelay = "10ms";

  CommandLine cmd;
  cmd.AddValue("TotalRunTimeSeconds",
               "Total running time of the simulation in seconds (> 20)",
               TotalRunTimeSeconds);
  cmd.AddValue(
      "Synchronized",
      "If set, the data publishing events from all nodes are synchronized",
      Synchronized);
  cmd.AddValue("LossRate", "Packet loss rate in the network", LossRate);
  cmd.AddValue("LinkDelay", "Delay of the underlying P2P channel", LinkDelay);
  cmd.Parse(argc, argv);

  NodeContainer nodes;
  nodes.Create(2);

  Config::SetDefault("ns3::PointToPointChannel::Delay", StringValue(LinkDelay));

  PointToPointHelper p2p;
  Ptr<RateErrorModel> rem = CreateObject<RateErrorModel>();
  rem->SetAttribute("ErrorRate", DoubleValue(LossRate));
  rem->SetAttribute("ErrorUnit", StringValue("ERROR_UNIT_PACKET"));
  p2p.Install(nodes.Get(0), nodes.Get(1));
  nodes.Get(0)->GetDevice(0)->SetAttribute("ReceiveErrorModel",
                                           PointerValue(rem));

  ndn::StackHelper ndnHelper;
  ndnHelper.SetDefaultRoutes(true);
  ndnHelper.InstallAll();

  ndn::StrategyChoiceHelper::InstallAll(::ndn::vsync::kSyncPrefix,
                                        "/localhost/nfd/strategy/multicast");

  Ptr<UniformRandomVariable> seed = CreateObject<UniformRandomVariable>();
  seed->SetAttribute("Min", DoubleValue(0.0));
  seed->SetAttribute("Max", DoubleValue(1000.0));

  for (int i = 0; i < 2; ++i) {
    ndn::AppHelper helper("ns3::ndn::vsync::SimpleNodeApp");
    helper.SetAttribute(
        "NodeID", StringValue("/N" + std::to_string(nodes.Get(i)->GetId())));
    if (!Synchronized)
      helper.SetAttribute("RandomSeed", UintegerValue(seed->GetInteger()));
    helper.Install(nodes.Get(i)).Start(Seconds(1.0));
    nodes.Get(i)->GetApplication(0)->TraceConnectWithoutContext(
        "DataEvent", MakeCallback(&DataEvent));
  }

  ndn::FibHelper::AddRoute(nodes.Get(0), ::ndn::vsync::kSyncPrefix,
                           nodes.Get(1), 1);
  ndn::FibHelper::AddRoute(nodes.Get(1), ::ndn::vsync::kSyncPrefix,
                           nodes.Get(0), 1);

  Simulator::Stop(Seconds(TotalRunTimeSeconds));

  Simulator::Run();
  Simulator::Destroy();

  double average_delay =
      std::accumulate(delays.begin(), delays.end(), 0.0,
                      [](double a, const decltype(delays)::value_type& b) {
                        return a + b.second.second - b.second.first;
                      }) /
      delays.size();

  std::cout << "Total number of data published is: " << delays.size()
            << std::endl;
  std::cout << "Average data propagation delay is: " << average_delay
            << " seconds." << std::endl;

  return 0;
}

}  // namespace ns3

int main(int argc, char* argv[]) { return ns3::main(argc, argv); }

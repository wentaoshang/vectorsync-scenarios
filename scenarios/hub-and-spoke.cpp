/* -*- Mode:C++; c-file-style:"google"; indent-tabs-mode:nil; -*- */

#include "simple-app.hpp"

#include <algorithm>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "ns3/core-module.h"
#include "ns3/ndnSIM-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/random-variable-stream.h"

NS_LOG_COMPONENT_DEFINE("ns3.ndn.vsync.scenarios.HubAndSpoke");

namespace ns3 {

std::unordered_map<std::string, std::pair<double, std::vector<double>>> delays;

static void DataEvent(std::string nid, std::shared_ptr<const ndn::Data> data,
                      bool is_local) {
  auto name = data->getName().toUri();
  NS_LOG_INFO("new_data_name=" << name << ", node_id=" << nid << ", is_local="
                               << (is_local ? "true" : "false"));

  double now = Simulator::Now().GetSeconds();

  auto& entry = delays[name];
  if (is_local)
    entry.first = now;
  else
    entry.second.push_back(now);
}

static void VectorClockChange(std::string nid, std::size_t idx,
                              const ::ndn::vsync::VersionVector& vc) {
  NS_LOG_INFO("node_id=\"" << nid << "\", node_index=" << idx
                           << ", vector_clock=" << vc);
}

static void ViewChange(std::string nid, const ::ndn::vsync::ViewID& vid,
                       const ::ndn::vsync::ViewInfo& vinfo) {
  NS_LOG_INFO("node_id=\"" << nid << "\", view_id=" << vid
                           << ", view_info=" << vinfo);
}

int main(int argc, char* argv[]) {
  Config::SetDefault("ns3::PointToPointNetDevice::DataRate",
                     StringValue("10Mbps"));
  Config::SetDefault("ns3::PointToPointChannel::Delay", StringValue("10ms"));
  Config::SetDefault("ns3::DropTailQueue::MaxPackets", StringValue("100"));

  int N = 10;
  double TotalRunTimeSeconds = 60.0;
  bool Synchronized = false;

  CommandLine cmd;
  cmd.AddValue("NumOfNodes", "Number of sync nodes in the group", N);
  cmd.AddValue("TotalRunTimeSeconds",
               "Total running time of the simulation in seconds",
               TotalRunTimeSeconds);
  cmd.AddValue(
      "Synchronized",
      "If set, the data publishing events from all nodes are synchronized",
      Synchronized);
  cmd.Parse(argc, argv);

  NodeContainer nodes;
  nodes.Create(N + 1);

  // Node 0 is central hub
  PointToPointHelper p2p;
  for (int i = 1; i <= N; ++i) p2p.Install(nodes.Get(0), nodes.Get(i));

  ndn::StackHelper ndnHelper;
  ndnHelper.SetDefaultRoutes(true);
  ndnHelper.InstallAll();

  ndn::StrategyChoiceHelper::InstallAll(::ndn::vsync::kSyncPrefix.toUri(),
                                        "/localhost/nfd/strategy/multicast");

  Ptr<UniformRandomVariable> seed = CreateObject<UniformRandomVariable>();
  seed->SetAttribute("Min", DoubleValue(0.0));
  seed->SetAttribute("Max", DoubleValue(1000.0));

  for (int i = 1; i <= N; ++i) {
    ndn::AppHelper helper("ns3::ndn::vsync::SimpleNodeApp");
    std::string nid = 'N' + std::to_string(i);
    helper.SetAttribute("NodeID", StringValue(nid));
    if (!Synchronized)
      helper.SetAttribute("RandomSeed", UintegerValue(seed->GetInteger()));
    helper.SetAttribute("StartTime", TimeValue(Seconds(1.0)));
    helper.SetAttribute("StopTime", TimeValue(Seconds(TotalRunTimeSeconds)));
    helper.Install(nodes.Get(i));

    ndn::FibHelper::AddRoute(nodes.Get(0), ::ndn::vsync::kSyncPrefix.toUri(),
                             nodes.Get(i), 1);
    std::string node_prefix = '/' + nid;
    ndn::FibHelper::AddRoute(nodes.Get(0), node_prefix, nodes.Get(i), 1);

    ndn::FibHelper::AddRoute(nodes.Get(i), "/", nodes.Get(0), 1);
    ndn::FibHelper::AddRoute(nodes.Get(i), ::ndn::vsync::kSyncPrefix.toUri(),
                             nodes.Get(0), 1);

    nodes.Get(i)->GetApplication(0)->TraceConnect(
        "VectorClock", nid, MakeCallback(&VectorClockChange));
    nodes.Get(i)->GetApplication(0)->TraceConnect("ViewChange", nid,
                                                  MakeCallback(&ViewChange));
    nodes.Get(i)->GetApplication(0)->TraceConnect("DataEvent", nid,
                                                  MakeCallback(&DataEvent));
  }

  Simulator::Stop(Seconds(TotalRunTimeSeconds));

  Simulator::Run();
  Simulator::Destroy();

  int count = 0;
  double average_delay = std::accumulate(
      delays.begin(), delays.end(), 0.0,
      [&count](double a, const decltype(delays)::value_type& b) {
        double gen_time = b.second.first;
        const auto& vec = b.second.second;
        count += vec.size();
        return a + std::accumulate(vec.begin(), vec.end(), 0.0,
                                   [gen_time](double c, double d) {
                                     return c + d - gen_time;
                                   });
      });
  average_delay /= count;

  std::cout << "Total number of data propagated is: " << count << std::endl;
  std::cout << "Average data propagation delay is: " << average_delay
            << " seconds." << std::endl;

  return 0;
}

}  // namespace ns3

int main(int argc, char* argv[]) { return ns3::main(argc, argv); }

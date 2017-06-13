/* -*- Mode:C++; c-file-style:"google"; indent-tabs-mode:nil; -*- */

#include "simple-app.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "ns3/core-module.h"
#include "ns3/ndnSIM-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/random-variable-stream.h"

NS_LOG_COMPONENT_DEFINE("ns3.ndn.vsync.scenarios.Line");

namespace ns3 {

std::unordered_map<std::string, std::pair<double, std::vector<double>>> delays;

static void DataEvent(std::string nid, std::shared_ptr<const ndn::Data> data,
                      bool is_local) {
  auto name = data->getName().toUri();
  /*
  NS_LOG_INFO("new_data_name=" << name << ", node_id=" << nid << ", is_local="
                               << (is_local ? "true" : "false"));
  */
  double now = Simulator::Now().GetSeconds();

  auto& entry = delays[name];
  if (is_local)
    entry.first = now;
  else
    entry.second.push_back(now);
}
/*
static void VectorClockChange(std::string nid, std::size_t idx,
                              const ::ndn::vsync::VersionVector& vc) {
  NS_LOG_INFO("node_id=\"" << nid << "\", node_index=" << idx
                           << ", vector_clock=" << vc);
}
*/
static void ViewChange(std::string nid, const ::ndn::vsync::ViewID& vid,
                       const ::ndn::vsync::ViewInfo& vinfo, bool is_leader) {
  NS_LOG_INFO("node_id=\"" << nid << "\", is_leader=" << (is_leader ? 'Y' : 'N')
                           << ", view_id=" << vid << ", view_info=" << vinfo);
}

int main(int argc, char* argv[]) {
  Config::SetDefault("ns3::PointToPointNetDevice::DataRate",
                     StringValue("100Mbps"));
  Config::SetDefault("ns3::DropTailQueue::MaxPackets", StringValue("100"));

  int N = 10;
  double TotalRunTimeSeconds = 100.0;
  bool Synchronized = false;
  int LinkDelayMS = 10;
  double DataRate = 1.0;

  CommandLine cmd;
  cmd.AddValue("NumOfNodes", "Number of sync nodes in the group (>= 2)", N);
  cmd.AddValue("TotalRunTimeSeconds",
               "Total running time of the simulation in seconds",
               TotalRunTimeSeconds);
  cmd.AddValue(
      "Synchronized",
      "If set, the data publishing events from all nodes are synchronized",
      Synchronized);
  cmd.AddValue("LinkDelayMS", "Delay of the underlying P2P channel in ms",
               LinkDelayMS);
  cmd.AddValue("DataRate", "Data publishing rate (packets per second)",
               DataRate);
  cmd.Parse(argc, argv);

  ::ndn::vsync::SetInterestLifetime(ndn::time::milliseconds(20 * LinkDelayMS),
                                    ndn::time::milliseconds(20 * LinkDelayMS));

  ::ndn::vsync::SetHeartbeatInterval(
      ndn::time::milliseconds(static_cast<int>(1000.0 / DataRate)));

  if (N < 2) return -1;

  NodeContainer nodes;
  nodes.Create(N);

  Config::SetDefault("ns3::PointToPointChannel::Delay",
                     TimeValue(MilliSeconds(LinkDelayMS)));

  PointToPointHelper p2p;
  for (int i = 0; i < N - 1; ++i) p2p.Install(nodes.Get(i), nodes.Get(i + 1));

  ndn::StackHelper ndnHelper;
  ndnHelper.setCsSize(1000);
  ndnHelper.InstallAll();

  ndn::StrategyChoiceHelper::InstallAll(::ndn::vsync::kSyncPrefix,
                                        "/localhost/nfd/strategy/multicast");

  ndn::GlobalRoutingHelper ndnGlobalRoutingHelper;
  ndnGlobalRoutingHelper.InstallAll();

  Ptr<UniformRandomVariable> seed = CreateObject<UniformRandomVariable>();
  seed->SetAttribute("Min", DoubleValue(0.0));
  seed->SetAttribute("Max", DoubleValue(1000.0));

  for (int i = 0; i < N; ++i) {
    ndn::AppHelper helper("ns3::ndn::vsync::SimpleNodeApp");
    std::string nid = "/N" + std::to_string(i);
    helper.SetAttribute("NodeID", StringValue(nid));
    if (!Synchronized)
      helper.SetAttribute("RandomSeed", UintegerValue(seed->GetInteger()));
    helper.Install(nodes.Get(i)).Start(Seconds(1.0));
    ndnGlobalRoutingHelper.AddOrigins(nid, nodes.Get(i));

    // nodes.Get(i)->GetApplication(0)->TraceConnect(
    //     "VectorClock", nid, MakeCallback(&VectorClockChange));
    nodes.Get(i)->GetApplication(0)->TraceConnect("ViewChange", nid,
                                                  MakeCallback(&ViewChange));
    nodes.Get(i)->GetApplication(0)->TraceConnect("DataEvent", nid,
                                                  MakeCallback(&DataEvent));
  }

  ndn::GlobalRoutingHelper::CalculateRoutes();

  ndn::FibHelper::AddRoute(nodes.Get(0), ::ndn::vsync::kSyncPrefix,
                           nodes.Get(1), 1);

  for (int i = 1; i < N - 1; ++i) {
    ndn::FibHelper::AddRoute(nodes.Get(i), ::ndn::vsync::kSyncPrefix,
                             nodes.Get(i - 1), 1);
    ndn::FibHelper::AddRoute(nodes.Get(i), ::ndn::vsync::kSyncPrefix,
                             nodes.Get(i + 1), 1);
  }

  ndn::FibHelper::AddRoute(nodes.Get(N - 1), ::ndn::vsync::kSyncPrefix,
                           nodes.Get(N - 2), 1);

  Simulator::Stop(Seconds(TotalRunTimeSeconds));

  Simulator::Run();
  Simulator::Destroy();

  std::string file_name =
      "results/LineD" + std::to_string(LinkDelayMS) + "N" + std::to_string(N);
  if (Synchronized) file_name += "Sync";

  std::fstream fs_sync_delay(file_name + "-sync-delay",
                             std::ios_base::out | std::ios_base::trunc);
  std::fstream fs_prop_delay(file_name + "-prop-delay",
                             std::ios_base::out | std::ios_base::trunc);

  int fully_synchronized_data = 0;
  double average_delay = 0.0;
  double max_delay = 0.0;
  for (auto iter = delays.begin(); iter != delays.end(); ++iter) {
    double gen_time = iter->second.first;
    const auto& vec = iter->second.second;
    if (vec.size() != N - 1) {
      std::cout << "gen_time: " << gen_time << ", group_size: " << N
                << ", vec.size: " << vec.size() << std::endl;
      continue;
    }
    ++fully_synchronized_data;
    double max_time = 0.0;
    for (auto iter2 = vec.begin(); iter2 != vec.end(); ++iter2) {
      if (*iter2 > max_time) max_time = *iter2;
      fs_prop_delay << gen_time << '\t' << *iter2 << std::endl;
    }

    fs_sync_delay << gen_time << '\t' << max_time << std::endl;

    double d = max_time - gen_time;
    if (max_delay < d) max_delay = d;
    average_delay += d;
  }
  average_delay /= fully_synchronized_data;

  fs_sync_delay.close();
  fs_prop_delay.close();

  std::cout << "Total number of data published is: " << delays.size()
            << std::endl;
  std::cout << "Total number of data fully synchronized is: "
            << fully_synchronized_data << std::endl;
  std::cout << "Max data propagation delay is: " << max_delay << " seconds."
            << std::endl;
  std::cout << "Average data propagation delay is: " << average_delay
            << " seconds." << std::endl;

  return 0;
}

}  // namespace ns3

int main(int argc, char* argv[]) { return ns3::main(argc, argv); }

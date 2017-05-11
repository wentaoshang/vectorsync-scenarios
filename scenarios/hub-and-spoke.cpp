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
                       const ::ndn::vsync::ViewInfo& vinfo, bool is_leader) {
  NS_LOG_INFO("node_id=\"" << nid << "\", is_leader=" << (is_leader ? 'Y' : 'N')
                           << ", view_id=" << vid << ", view_info=" << vinfo);
}

int main(int argc, char* argv[]) {
  Config::SetDefault("ns3::PointToPointNetDevice::DataRate",
                     StringValue("100Mbps"));
  Config::SetDefault("ns3::DropTailQueue::MaxPackets", StringValue("100"));
  Config::SetDefault("ns3::RateErrorModel::ErrorUnit",
                     StringValue("ERROR_UNIT_PACKET"));

  int N = 10;
  double TotalRunTimeSeconds = 100.0;
  bool Synchronized = false;
  double LossRate = 0.0;
  bool LossyMode = false;
  std::string LinkDelay = "100ms";
  int LeavingNodes = 0;
  double DataRate = 1.0;

  CommandLine cmd;
  cmd.AddValue("NumOfNodes", "Number of sync nodes in the group", N);
  cmd.AddValue("TotalRunTimeSeconds",
               "Total running time of the simulation in seconds",
               TotalRunTimeSeconds);
  cmd.AddValue(
      "Synchronized",
      "If set, the data publishing events from all nodes are synchronized",
      Synchronized);
  cmd.AddValue("LossRate", "Packet loss rate in the network", LossRate);
  cmd.AddValue("LossyMode", "If set, the sync nodes will enable lossy mode",
               LossyMode);
  cmd.AddValue("LinkDelay", "Delay of the underlying P2P channel", LinkDelay);
  cmd.AddValue("LeavingNodes",
               "Number of nodes randomly leaving the group after 20s",
               LeavingNodes);
  cmd.AddValue("DataRate", "Data publishing rate (packets per second)",
               DataRate);
  cmd.Parse(argc, argv);

  NodeContainer nodes;
  nodes.Create(N + 1);

  Config::SetDefault("ns3::PointToPointChannel::Delay", StringValue(LinkDelay));

  // Node 0 is central hub
  PointToPointHelper p2p;
  Ptr<RateErrorModel> rem = CreateObject<RateErrorModel>();
  rem->SetAttribute("ErrorRate", DoubleValue(LossRate));
  rem->SetAttribute("ErrorUnit", StringValue("ERROR_UNIT_PACKET"));
  for (int i = 1; i <= N; ++i) {
    p2p.Install(nodes.Get(0), nodes.Get(i));
    nodes.Get(i)->GetDevice(0)->SetAttribute("ReceiveErrorModel",
                                             PointerValue(rem));
  }

  ndn::StackHelper ndnHelper;
  ndnHelper.setCsSize(1000);
  ndnHelper.InstallAll();

  ndn::StrategyChoiceHelper::InstallAll(::ndn::vsync::kSyncPrefix,
                                        "/localhost/nfd/strategy/multicast");

  Ptr<UniformRandomVariable> seed = CreateObject<UniformRandomVariable>();
  seed->SetAttribute("Min", DoubleValue(0.0));
  seed->SetAttribute("Max", DoubleValue(1000.0));

  Ptr<UniformRandomVariable> stop_time = CreateObject<UniformRandomVariable>();
  stop_time->SetAttribute("Min", DoubleValue(20.0));
  stop_time->SetAttribute("Max", DoubleValue(TotalRunTimeSeconds));

  std::vector<::ndn::vsync::MemberInfo> mlist;
  for (int i = 1; i <= N; ++i) {
    std::string nid = 'N' + std::to_string(i);
    mlist.push_back({nid, "/"});
  }
  ::ndn::vsync::ViewInfo vinfo(mlist);
  std::string vinfo_proto;
  vinfo.Encode(vinfo_proto);

  for (int i = 1; i <= N; ++i) {
    ndn::AppHelper helper("ns3::ndn::vsync::SimpleNodeApp");
    std::string nid = 'N' + std::to_string(i);
    helper.SetAttribute("NodeID", StringValue(nid));
    helper.SetAttribute("ViewInfo", StringValue(vinfo_proto));
    if (LossyMode) helper.SetAttribute("LossyMode", BooleanValue(true));
    if (!Synchronized)
      helper.SetAttribute("RandomSeed", UintegerValue(seed->GetInteger()));
    helper.SetAttribute("DataRate", DoubleValue(DataRate));
    helper.SetAttribute("StartTime", TimeValue(Seconds(1.0)));
    if (i <= LeavingNodes)
      helper.SetAttribute("StopTime",
                          TimeValue(Seconds(stop_time->GetValue())));
    else
      helper.SetAttribute("StopTime", TimeValue(Seconds(TotalRunTimeSeconds)));
    helper.Install(nodes.Get(i));

    ndn::FibHelper::AddRoute(nodes.Get(0), ::ndn::vsync::kSyncPrefix,
                             nodes.Get(i), 1);
    std::string node_prefix = '/' + nid;
    ndn::FibHelper::AddRoute(nodes.Get(0), node_prefix, nodes.Get(i), 1);

    ndn::FibHelper::AddRoute(nodes.Get(i), "/", nodes.Get(0), 1);
    ndn::FibHelper::AddRoute(nodes.Get(i), ::ndn::vsync::kSyncPrefix,
                             nodes.Get(0), 1);

    nodes.Get(i)->GetApplication(0)->TraceConnect(
        "VectorClock", nid, MakeCallback(&VectorClockChange));
    nodes.Get(i)->GetApplication(0)->TraceConnect("ViewChange", nid,
                                                  MakeCallback(&ViewChange));
    nodes.Get(i)->GetApplication(0)->TraceConnect("DataEvent", nid,
                                                  MakeCallback(&DataEvent));
  }

  Simulator::Stop(Seconds(TotalRunTimeSeconds));

  ndn::L3RateTracer::InstallAll("rate-trace.txt",
                                Seconds(TotalRunTimeSeconds - 0.5));

  Simulator::Run();
  Simulator::Destroy();

  std::string file_name = "results/D" + LinkDelay + "N" + std::to_string(N);
  if (DataRate != 1.0) file_name += "DR" + std::to_string(DataRate);
  if (Synchronized) file_name += "Sync";
  if (LossyMode) file_name += "LM";
  if (LossRate > 0.0) file_name += "LR" + std::to_string(LossRate);
  if (LeavingNodes > 0) file_name += "LN" + std::to_string(LeavingNodes);
  std::fstream fs(file_name, std::ios_base::out | std::ios_base::trunc);

  int count = 0;
  double average_delay = std::accumulate(
      delays.begin(), delays.end(), 0.0,
      [&count, &fs](double a, const decltype(delays)::value_type& b) {
        double gen_time = b.second.first;
        const auto& vec = b.second.second;
        count += vec.size();
        return a + std::accumulate(vec.begin(), vec.end(), 0.0,
                                   [gen_time, &fs](double c, double d) {
                                     fs << (d - gen_time) << std::endl;
                                     return c + d - gen_time;
                                   });
      });
  average_delay /= count;

  fs.close();

  std::cout << "Total number of data published is: " << delays.size()
            << std::endl;
  std::cout << "Total number of data propagated is: " << count << std::endl;
  std::cout << "Average data propagation delay is: " << average_delay
            << " seconds." << std::endl;

  return 0;
}

}  // namespace ns3

int main(int argc, char* argv[]) { return ns3::main(argc, argv); }

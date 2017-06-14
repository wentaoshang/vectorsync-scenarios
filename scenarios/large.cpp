/* -*- Mode:C++; c-file-style:"google"; indent-tabs-mode:nil; -*- */

#include "simple-app.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

#include "ns3/core-module.h"
#include "ns3/ndnSIM-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/random-variable-stream.h"

NS_LOG_COMPONENT_DEFINE("ns3.ndn.vsync.scenarios.Large");

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

std::map<::ndn::vsync::ViewID, std::pair<double, std::vector<double>>,
         ::ndn::vsync::VIDCompare>
    view_change_delays;

static void ViewChange(std::string nid, const ::ndn::vsync::ViewID& vid,
                       const ::ndn::vsync::ViewInfo& vinfo, bool is_leader) {
  NS_LOG_INFO("node_id=\"" << nid << "\", is_leader=" << (is_leader ? 'Y' : 'N')
                           << ", view_id=" << vid << ", view_info=" << vinfo);

  double now = Simulator::Now().GetSeconds();

  auto& entry = view_change_delays[vid];
  if (is_leader)
    entry.first = now;
  else
    entry.second.push_back(now);
}

static void NodeStop(std::string nid) {
  NS_LOG_INFO("node /" << nid << " stops");
}

int main(int argc, char* argv[]) {
  Config::SetDefault("ns3::DropTailQueue::MaxPackets", StringValue("100"));

  double TotalRunTimeSeconds = 120.0;
  double LossRate = 0.0;
  bool Synchronized = false;
  double DataRate = 1.0;
  int LeavingNodes = 0;

  ::ndn::vsync::SetInterestLifetime(ndn::time::milliseconds(500),
                                    ndn::time::milliseconds(500));

  CommandLine cmd;
  cmd.AddValue("TotalRunTimeSeconds",
               "Total running time of the simulation in seconds",
               TotalRunTimeSeconds);
  cmd.AddValue("LossRate", "Packet loss rate in the network", LossRate);
  cmd.AddValue(
      "Synchronized",
      "If set, the data publishing events from all nodes are synchronized",
      Synchronized);
  cmd.AddValue("LeavingNodes",
               "Number of nodes randomly leaving the group after 10s",
               LeavingNodes);
  cmd.AddValue("DataRate", "Data publishing rate (packets per second)",
               DataRate);
  cmd.Parse(argc, argv);

  ::ndn::vsync::SetHeartbeatInterval(
      ndn::time::milliseconds(static_cast<int>(1000.0 / DataRate)));

  AnnotatedTopologyReader topologyReader("", 25);
  topologyReader.SetFileName("topologies/6461.r0-conv-annotated.txt");
  topologyReader.Read();

  // Install Ndn stack on all nodes
  ndn::StackHelper ndnHelper;
  ndnHelper.setCsSize(5000);
  ndnHelper.InstallAll();

  // Installing global routing interface on all nodes
  ndn::GlobalRoutingHelper ndnGlobalRoutingHelper;
  ndnGlobalRoutingHelper.InstallAll();

  ndn::StrategyChoiceHelper::InstallAll(::ndn::vsync::kSyncPrefix,
                                        "/localhost/nfd/strategy/multicast");

  Ptr<UniformRandomVariable> seed = CreateObject<UniformRandomVariable>();
  seed->SetAttribute("Min", DoubleValue(0.0));
  seed->SetAttribute("Max", DoubleValue(1000.0));

  Ptr<UniformRandomVariable> stop_time = CreateObject<UniformRandomVariable>();
  stop_time->SetAttribute("Min", DoubleValue(10.0));
  stop_time->SetAttribute("Max", DoubleValue(TotalRunTimeSeconds));

  PointToPointHelper p2p;
  Ptr<RateErrorModel> rem = CreateObject<RateErrorModel>();
  rem->SetAttribute("ErrorRate", DoubleValue(LossRate));
  rem->SetAttribute("ErrorUnit", StringValue("ERROR_UNIT_PACKET"));
  Config::Set(
      "/NodeList/*/DeviceList/*/$ns3::PointToPointNetDevice/ReceiveErrorModel",
      PointerValue(rem));

  std::vector<std::string> nodes{"leaf-505", "leaf-687", "leaf-741", "leaf-580",
                                 "leaf-463", "leaf-721", "leaf-486", "leaf-675",
                                 "leaf-799", "leaf-525"};

  std::vector<::ndn::vsync::MemberInfo> mlist;
  for (size_t i = 0; i < nodes.size(); ++i) {
    mlist.push_back({::ndn::Name('/' + nodes[i])});
  }
  ::ndn::vsync::ViewInfo vinfo(mlist);
  std::string vinfo_proto;
  vinfo.Encode(vinfo_proto);

  std::map<double, int> group_size;

  for (size_t i = 0; i < nodes.size(); ++i) {
    const std::string& nid = nodes[i];
    Ptr<Node> node = Names::Find<Node>(nid);

    ndn::AppHelper helper("ns3::ndn::vsync::SimpleNodeApp");
    helper.SetAttribute("NodeID", StringValue('/' + nid));
    helper.SetAttribute("ViewInfo", StringValue(vinfo_proto));
    helper.SetAttribute("StartTime", TimeValue(Seconds(1.0)));
    if (i < LeavingNodes) {
      double st = stop_time->GetValue();
      group_size[st] = 10;
      std::cout << "node /" << nid << " leaves at " << st << std::endl;
      Simulator::Schedule(Seconds(st), NodeStop, nid);
      helper.SetAttribute("StopTime", TimeValue(Seconds(st)));
    } else {
      helper.SetAttribute("StopTime", TimeValue(Seconds(TotalRunTimeSeconds)));
    }
    helper.SetAttribute("DataRate", DoubleValue(DataRate));
    if (!Synchronized)
      helper.SetAttribute("RandomSeed", UintegerValue(seed->GetInteger()));
    helper.Install(node);

    ndnGlobalRoutingHelper.AddOrigins('/' + nid, node);
    ndnGlobalRoutingHelper.AddOrigins(::ndn::vsync::kSyncPrefix.toUri(), node);

    node->GetApplication(0)->TraceConnect("DataEvent", nid,
                                          MakeCallback(&DataEvent));

    node->GetApplication(0)->TraceConnect("ViewChange", nid,
                                          MakeCallback(&ViewChange));
  }

  ndn::GlobalRoutingHelper::CalculateRoutes();

  Simulator::Stop(Seconds(TotalRunTimeSeconds));
  group_size[TotalRunTimeSeconds] = 10;
  int gk = 10;
  for (auto iter = group_size.begin(); iter != group_size.end(); ++iter) {
    iter->second = gk;
    --gk;
  }

  std::string file_name =
      "results/VS-LargeRunTime" + std::to_string(TotalRunTimeSeconds);
  if (Synchronized) file_name += "Sync";
  if (LossRate > 0.0) file_name += "LR" + std::to_string(LossRate);
  if (DataRate != 1.0) file_name += "DR" + std::to_string(DataRate);
  if (LeavingNodes > 0) file_name += "LN" + std::to_string(LeavingNodes);

  ndn::L3RateTracer::InstallAll(file_name + "-rate-trace.txt",
                                Seconds(TotalRunTimeSeconds - 0.1));

  Simulator::Run();
  Simulator::Destroy();

  std::fstream fs(file_name, std::ios_base::out | std::ios_base::trunc);

  int fully_synchronized_data = 0;
  double average_delay = 0.0;
  double max_delay = 0.0;
  for (auto iter = delays.begin(); iter != delays.end(); ++iter) {
    const auto& s = iter->first;
    double gen_time = iter->second.first;
    const auto& vec = iter->second.second;
    int gs = group_size.upper_bound(gen_time)->second;
    if (vec.size() != gs - 1 || vec.size() == 0) {
      std::cout << "name: " << s << ", gen_time: " << gen_time
                << ", group_size: " << gs << ", vec.size: " << vec.size()
                << std::endl;
      continue;
    }
    ++fully_synchronized_data;
    double max_time = 0.0;
    for (auto iter2 = vec.begin(); iter2 != vec.end(); ++iter2) {
      if (*iter2 > max_time) max_time = *iter2;
    }

    // Output: gen_time at the 1st column; max_time at the 2nd column; then
    // followed by individual data receiving time
    fs << gen_time << '\t' << max_time;
    for (auto iter2 = vec.begin(); iter2 != vec.end(); ++iter2) {
      if (*iter2 > max_time) max_time = *iter2;
      fs << '\t' << *iter2;
    }
    fs << std::endl;

    double d = max_time - gen_time;
    if (max_delay < d) max_delay = d;
    average_delay += d;
  }
  average_delay /= fully_synchronized_data;

  fs.close();

  std::cout << "Total number of data published is: " << delays.size()
            << std::endl;
  std::cout << "Total number of data fully synchronized is: "
            << fully_synchronized_data << std::endl;
  std::cout << "Max data propagation delay is: " << max_delay << " seconds."
            << std::endl;
  std::cout << "Average data propagation delay is: " << average_delay
            << " seconds." << std::endl;

  double max_view_change_delay = 0.0;
  for (auto iter = view_change_delays.begin(); iter != view_change_delays.end();
       ++iter) {
    const auto& vid = iter->first;
    double start = iter->second.first;
    const auto& vec = iter->second.second;
    for (auto iter2 = vec.begin(); iter2 != vec.end(); ++iter2) {
      /*
      std::cout << "ViewID: " << vid << ", start: " << start
                << ", join: " << *iter2 << std::endl;
      */
      double d = *iter2 - start;
      if (d > max_view_change_delay) max_view_change_delay = d;
    }
  }
  std::cout << "Max view change delay is: " << max_view_change_delay
            << " seconds." << std::endl;

  return 0;
}

}  // namespace ns3

int main(int argc, char* argv[]) { return ns3::main(argc, argv); }

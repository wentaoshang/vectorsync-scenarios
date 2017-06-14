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

NS_LOG_COMPONENT_DEFINE("ns3.ndn.vsync.scenarios.HubAndSpoke");

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
static void VectorChange(std::string nid, std::size_t idx,
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

static void NodeStop(std::string nid) {
  NS_LOG_INFO("node " << nid << " stops");
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
  int LinkDelayMS = 10;
  int LeavingNodes = 0;
  double DataRate = 1.0;
  int HBMultiple = 1;

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
  cmd.AddValue("LinkDelayMS", "Delay of the underlying P2P channel in ms",
               LinkDelayMS);
  cmd.AddValue("LeavingNodes", "Number of nodes randomly leaving the group",
               LeavingNodes);
  cmd.AddValue("DataRate", "Data publishing rate (packets per second)",
               DataRate);
  cmd.AddValue("HBMultiple",
               "Heartbeat interval as a multiple of the data interval",
               HBMultiple);
  cmd.Parse(argc, argv);

  ::ndn::vsync::SetInterestLifetime(ndn::time::milliseconds(5 * LinkDelayMS),
                                    ndn::time::milliseconds(5 * LinkDelayMS));

  ::ndn::vsync::SetHeartbeatInterval(ndn::time::milliseconds(
      HBMultiple * static_cast<int>(1000.0 / DataRate)));

  NodeContainer nodes;
  nodes.Create(N + 1);

  Config::SetDefault("ns3::PointToPointChannel::Delay",
                     TimeValue(MilliSeconds(LinkDelayMS)));

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
  stop_time->SetAttribute("Min", DoubleValue(10.0));
  stop_time->SetAttribute("Max", DoubleValue(TotalRunTimeSeconds));

  std::vector<::ndn::vsync::MemberInfo> mlist;
  for (int i = 1; i <= N; ++i) {
    std::string nid = 'N' + std::to_string(i);
    mlist.push_back({::ndn::Name('/' + nid)});
  }
  ::ndn::vsync::ViewInfo vinfo(mlist);
  std::string vinfo_proto;
  vinfo.Encode(vinfo_proto);

  std::map<double, int> group_size;

  for (int i = 1; i <= N; ++i) {
    ndn::AppHelper helper("ns3::ndn::vsync::SimpleNodeApp");
    std::string nid = "/N" + std::to_string(i);
    helper.SetAttribute("NodeID", StringValue(nid));
    helper.SetAttribute("ViewInfo", StringValue(vinfo_proto));
    if (!Synchronized)
      helper.SetAttribute("RandomSeed", UintegerValue(seed->GetInteger()));
    helper.SetAttribute("DataRate", DoubleValue(DataRate));
    helper.SetAttribute("StartTime", TimeValue(Seconds(1.0)));
    if (i <= LeavingNodes) {
      double st = stop_time->GetValue();
      group_size[st] = 0;
      std::cout << "node " << nid << " leaves at " << st << std::endl;
      Simulator::Schedule(Seconds(st), NodeStop, nid);
      helper.SetAttribute("StopTime", TimeValue(Seconds(st)));
    } else {
      helper.SetAttribute("StopTime", TimeValue(Seconds(TotalRunTimeSeconds)));
    }
    helper.Install(nodes.Get(i));

    ndn::FibHelper::AddRoute(nodes.Get(0), ::ndn::vsync::kSyncPrefix,
                             nodes.Get(i), 1);
    ndn::FibHelper::AddRoute(nodes.Get(0), nid, nodes.Get(i), 1);

    ndn::FibHelper::AddRoute(nodes.Get(i), "/", nodes.Get(0), 1);
    ndn::FibHelper::AddRoute(nodes.Get(i), ::ndn::vsync::kSyncPrefix,
                             nodes.Get(0), 1);

    // nodes.Get(i)->GetApplication(0)->TraceConnect("VectorChange", nid,
    //                                               MakeCallback(&VectorChange));
    nodes.Get(i)->GetApplication(0)->TraceConnect("ViewChange", nid,
                                                  MakeCallback(&ViewChange));
    nodes.Get(i)->GetApplication(0)->TraceConnect("DataEvent", nid,
                                                  MakeCallback(&DataEvent));
  }

  Simulator::Stop(Seconds(TotalRunTimeSeconds));
  group_size[TotalRunTimeSeconds] = 0;
  int gk = N;
  for (auto iter = group_size.begin(); iter != group_size.end(); ++iter) {
    iter->second = gk;
    --gk;
  }

  std::string file_name =
      "results/D" + std::to_string(LinkDelayMS) + "N" + std::to_string(N);
  if (Synchronized) file_name += "Sync";
  if (LossRate > 0.0) file_name += "LR" + std::to_string(LossRate);
  if (DataRate != 1.0) file_name += "DR" + std::to_string(DataRate);
  if (LeavingNodes > 0) file_name += "LN" + std::to_string(LeavingNodes);
  if (HBMultiple != 1) file_name += "HB" + std::to_string(HBMultiple);

  ndn::L3RateTracer::InstallAll(file_name + "-rate-trace.txt",
                                Seconds(TotalRunTimeSeconds - 0.1));

  Simulator::Run();
  Simulator::Destroy();

  std::fstream fs_sync_delay(file_name + "-sync-delay",
                             std::ios_base::out | std::ios_base::trunc);
  std::fstream fs_prop_delay(file_name + "-prop-delay",
                             std::ios_base::out | std::ios_base::trunc);

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

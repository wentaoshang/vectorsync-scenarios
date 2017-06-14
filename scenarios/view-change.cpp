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

NS_LOG_COMPONENT_DEFINE("ns3.ndn.vsync.scenarios.ViewChange");

namespace ns3 {

static void ViewChange(std::string nid, const ::ndn::vsync::ViewID& vid,
                       const ::ndn::vsync::ViewInfo& vinfo, bool is_leader) {
  NS_LOG_INFO("node_id=\"" << nid << "\", is_leader=" << (is_leader ? 'Y' : 'N')
                           << ", view_id=" << vid << ", view_info=" << vinfo);
}

static void NodeStop(std::string nid) {
  NS_LOG_INFO("node /" << nid << " stops");
}

int main(int argc, char* argv[]) {
  Config::SetDefault("ns3::DropTailQueue::MaxPackets", StringValue("100"));

  int N = 10;
  double TotalRunTimeSeconds = 30.0;
  double LossRate = 0.0;
  bool Synchronized = false;
  std::string LinkDelay = "100ms";
  int LeavingNodes = 0;
  double DataRate = 1.0;

  ::ndn::vsync::SetInterestLifetime(ndn::time::milliseconds(100),
                                    ndn::time::milliseconds(100));

  ::ndn::vsync::SetHeartbeatInterval(ndn::time::milliseconds(2000));

  CommandLine cmd;
  cmd.AddValue("NumOfNodes", "Number of sync nodes in the group", N);
  cmd.AddValue("TotalRunTimeSeconds",
               "Total running time of the simulation in seconds",
               TotalRunTimeSeconds);
  cmd.AddValue("LossRate", "Packet loss rate in the network", LossRate);
  cmd.AddValue(
      "Synchronized",
      "If set, the data publishing events from all nodes are synchronized",
      Synchronized);
  cmd.AddValue("LeavingNodes",
               "Number of nodes randomly leaving the group after 20s",
               LeavingNodes);
  cmd.AddValue("DataRate", "Data publishing rate (packets per second)",
               DataRate);
  cmd.Parse(argc, argv);

  Config::SetDefault("ns3::PointToPointChannel::Delay", StringValue(LinkDelay));

  AnnotatedTopologyReader topologyReader("", 25);
  topologyReader.SetFileName("topologies/campus.txt");
  topologyReader.Read();

  // Install Ndn stack on all nodes
  ndn::StackHelper ndnHelper;
  ndnHelper.setCsSize(2000);
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
  stop_time->SetAttribute("Min", DoubleValue(20.0));
  stop_time->SetAttribute("Max", DoubleValue(TotalRunTimeSeconds));

  // Node 0 is central hub
  PointToPointHelper p2p;
  Ptr<RateErrorModel> rem = CreateObject<RateErrorModel>();
  rem->SetAttribute("ErrorRate", DoubleValue(LossRate));
  rem->SetAttribute("ErrorUnit", StringValue("ERROR_UNIT_PACKET"));
  Config::Set(
      "/NodeList/*/DeviceList/*/$ns3::PointToPointNetDevice/ReceiveErrorModel",
      PointerValue(rem));

  std::vector<::ndn::vsync::MemberInfo> mlist;
  for (int i = 1; i <= 10; ++i) {
    std::string nid = 'n' + std::to_string(i);
    mlist.push_back({::ndn::Name('/' + nid)});
  }
  ::ndn::vsync::ViewInfo vinfo(mlist);
  std::string vinfo_proto;
  vinfo.Encode(vinfo_proto);

  for (int i = 1; i <= 10; ++i) {
    std::string nid = 'n' + std::to_string(i);
    Ptr<Node> node = Names::Find<Node>(nid);

    ndn::AppHelper helper("ns3::ndn::vsync::SimpleNodeApp");
    helper.SetAttribute("NodeID", StringValue('/' + nid));
    helper.SetAttribute("ViewInfo", StringValue(vinfo_proto));
    helper.SetAttribute("StartTime", TimeValue(Seconds(1.0)));
    if (i <= LeavingNodes) {
      double st = stop_time->GetValue();
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

    node->GetApplication(0)->TraceConnect("ViewChange", nid,
                                          MakeCallback(&ViewChange));
  }

  ndn::GlobalRoutingHelper::CalculateRoutes();

  Simulator::Stop(Seconds(TotalRunTimeSeconds));

  // std::string file_name = "results/D" + LinkDelay + "N" + std::to_string(N);
  // if (DataRate != 1.0) file_name += "DR" + std::to_string(DataRate);
  // if (Synchronized) file_name += "Sync";
  // if (LossyMode) file_name += "LM";
  // if (LossRate > 0.0) file_name += "LR" + std::to_string(LossRate);
  // if (LeavingNodes > 0) file_name += "LN" + std::to_string(LeavingNodes);
  // std::fstream fs(file_name, std::ios_base::out | std::ios_base::trunc);

  // ndn::L3RateTracer::InstallAll("rate-trace.txt",
  //                               Seconds(TotalRunTimeSeconds - 0.5));

  Simulator::Run();
  Simulator::Destroy();

  // int count = 0;
  // double average_delay = std::accumulate(
  //     delays.begin(), delays.end(), 0.0,
  //     [&count, &fs](double a, const decltype(delays)::value_type& b) {
  //       double gen_time = b.second.first;
  //       const auto& vec = b.second.second;
  //       count += vec.size();
  //       return a + std::accumulate(vec.begin(), vec.end(), 0.0,
  //                                  [gen_time, &fs](double c, double d) {
  //                                    fs << (d - gen_time) << std::endl;
  //                                    return c + d - gen_time;
  //                                  });
  //     });
  // average_delay /= count;

  // fs.close();

  // std::cout << "Total number of data published is: " << delays.size()
  //           << std::endl;
  // std::cout << "Total number of data propagated is: " << count << std::endl;
  // std::cout << "Average data propagation delay is: " << average_delay
  //           << " seconds." << std::endl;

  return 0;
}

}  // namespace ns3

int main(int argc, char* argv[]) { return ns3::main(argc, argv); }

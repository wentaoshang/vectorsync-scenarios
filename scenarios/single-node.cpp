/* -*- Mode:C++; c-file-style:"google"; indent-tabs-mode:nil; -*- */

#include "simple-starter.hpp"
#include "simple.hpp"

#include "ns3/core-module.h"
#include "ns3/ndnSIM-module.h"
#include "ns3/network-module.h"

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED(SimpleNodeStarter);

int main(int argc, char* argv[]) {
  CommandLine cmd;
  cmd.Parse(argc, argv);

  Ptr<Node> node = CreateObject<Node>();

  ndn::StackHelper ndnHelper;
  ndnHelper.Install(node);

  ndn::AppHelper appHelper("SimpleNodeStarter");
  appHelper.SetAttribute("NodeID", StringValue("B"));
  appHelper.Install(node).Start(Seconds(1.5));

  Simulator::Stop(Seconds(60.0));

  Simulator::Run();
  Simulator::Destroy();

  return 0;
}

}  // namespace ns3

int main(int argc, char* argv[]) { return ns3::main(argc, argv); }

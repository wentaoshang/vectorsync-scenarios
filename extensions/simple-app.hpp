/* -*- Mode:C++; c-file-style:"google"; indent-tabs-mode:nil; -*- */

#ifndef SIMPLE_APP_HPP_
#define SIMPLE_APP_HPP_

#include "ns3/application.h"
#include "ns3/ndnSIM/helper/ndn-stack-helper.hpp"
#include "ns3/string.h"

#include "simple.hpp"

namespace ns3 {
namespace ndn {
namespace vsync {

class SimpleNodeApp : public Application {
 public:
  static TypeId GetTypeId() {
    static TypeId tid =
        TypeId("ns3::ndn::vsync::SimpleNodeApp")
            .SetParent<Application>()
            .AddConstructor<SimpleNodeApp>()
            .AddAttribute("NodeID", "Unique ID for the node in the sync group.",
                          StringValue(""),
                          MakeStringAccessor(&SimpleNodeApp::node_id_),
                          MakeStringChecker())
            .AddAttribute("RoutingPrefix", "Routing prefix for the sync node.",
                          StringValue("/"),
                          MakeStringAccessor(&SimpleNodeApp::routing_prefix_),
                          MakeStringChecker());

    return tid;
  }

 protected:
  virtual void StartApplication() {
    node_.reset(new ::ndn::vsync::app::SimpleNode(
        node_id_, routing_prefix_, ndn::StackHelper::getKeyChain()));
  }

  virtual void StopApplication() { node_.reset(); }

 private:
  std::unique_ptr<::ndn::vsync::app::SimpleNode> node_;
  std::string node_id_;
  std::string routing_prefix_;
};

}  // namespace vsync
}  // namespace ndn
}  // namespace ns3

#endif  // SIMPLE_APP_HPP_

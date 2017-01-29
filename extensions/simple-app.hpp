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
                          StringValue("A"),
                          MakeStringAccessor(&SimpleNodeApp::node_id_),
                          MakeStringChecker())
            .AddAttribute("NodePrefix", "Unicast prefix for the sync node.",
                          StringValue("/"),
                          MakeStringAccessor(&SimpleNodeApp::node_prefix_),
                          MakeStringChecker());

    return tid;
  }

 protected:
  virtual void StartApplication() {
    node_.reset(new ::ndn::vsync::app::SimpleNode(
        node_id_, node_prefix_, ndn::StackHelper::getKeyChain()));
  }

  virtual void StopApplication() {
    // Stop and destroy the instance of the app
    node_.reset();
  }

 private:
  std::unique_ptr<::ndn::vsync::app::SimpleNode> node_;
  std::string node_id_;
  std::string node_prefix_;
};

}  // namespace vsync
}  // namespace ndn
}  // namespace ns3

#endif  // SIMPLE_APP_HPP_

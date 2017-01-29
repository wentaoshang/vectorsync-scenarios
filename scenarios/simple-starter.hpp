/* -*- Mode:C++; c-file-style:"google"; indent-tabs-mode:nil; -*- */

#ifndef SIMPLE_STARTER_HPP_
#define SIMPLE_STARTER_HPP_

#include "ns3/application.h"
#include "ns3/ndnSIM/helper/ndn-stack-helper.hpp"
#include "ns3/string.h"

#include "simple.hpp"

namespace ns3 {

class SimpleNodeStarter : public Application {
 public:
  static TypeId GetTypeId() {
    static TypeId tid =
        TypeId("SimpleNodeStarter")
            .SetParent<Application>()
            .AddConstructor<SimpleNodeStarter>()
            .AddAttribute("NodeID", "Unique ID for the node in the sync group.",
                          StringValue("A"),
                          MakeStringAccessor(&SimpleNodeStarter::node_id_),
                          MakeStringChecker())
            .AddAttribute("NodePrefix", "Unicast prefix for the sync node.",
                          StringValue("/"),
                          MakeStringAccessor(&SimpleNodeStarter::node_prefix_),
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

}  // namespace ns3

#endif  // SIMPLE_STARTER_HPP_

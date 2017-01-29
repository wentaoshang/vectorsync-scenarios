/* -*- Mode:C++; c-file-style:"google"; indent-tabs-mode:nil; -*- */

#ifndef SIMPLE_STARTER_HPP_
#define SIMPLE_STARTER_HPP_

#include "ns3/application.h"
#include "ns3/ndnSIM/helper/ndn-stack-helper.hpp"

#include "simple.hpp"

namespace ns3 {

class SimpleNodeStarter : public Application {
 public:
  static TypeId GetTypeId() {
    static TypeId tid = TypeId("SimpleNodeStarter")
                            .SetParent<Application>()
                            .AddConstructor<SimpleNodeStarter>();

    return tid;
  }

 protected:
  virtual void StartApplication() {
    instance_.reset(new ::ndn::vsync::app::SimpleNode(
        "A", "/test", ndn::StackHelper::getKeyChain()));
  }

  virtual void StopApplication() {
    // Stop and destroy the instance of the app
    instance_.reset();
  }

 private:
  std::unique_ptr<::ndn::vsync::app::SimpleNode> instance_;
};

}  // namespace ns3

#endif  // SIMPLE_STARTER_HPP_

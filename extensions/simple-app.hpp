/* -*- Mode:C++; c-file-style:"google"; indent-tabs-mode:nil; -*- */

#ifndef SIMPLE_APP_HPP_
#define SIMPLE_APP_HPP_

#include "ns3/application.h"
#include "ns3/ndnSIM/helper/ndn-stack-helper.hpp"
#include "ns3/string.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/traced-callback.h"

#include "simple.hpp"

namespace ns3 {
namespace ndn {
namespace vsync {

class SimpleNodeApp : public Application {
 public:
  typedef void (*VectorClockTraceCallback)(const ::ndn::vsync::VersionVector&);
  typedef void (*ViewIDTraceCallback)(const ::ndn::vsync::ViewID&);
  typedef void (*DataEventTraceCallback)(std::shared_ptr<const ndn::Data>,
                                         bool);

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
                          MakeStringChecker())
            .AddTraceSource(
                "VectorClock", "Vector clock of the sync node.",
                MakeTraceSourceAccessor(&SimpleNodeApp::vector_clock_trace_),
                "ns3::ndn::vsync::SimpleNodeApp::VectorClockTraceCallback")
            .AddTraceSource(
                "ViewID", "View ID of the sync node.",
                MakeTraceSourceAccessor(&SimpleNodeApp::view_id_trace_),
                "ns3::ndn::vsync::SimpleNodeApp::ViewIDTraceCallback")
            .AddTraceSource(
                "DataEvent",
                "Event of publishing or receiving new data in the sync node.",
                MakeTraceSourceAccessor(&SimpleNodeApp::data_event_trace_),
                "ns3::ndn::vsync::SimpleNodeApp::DataEventTraceCallback");

    return tid;
  }

 protected:
  virtual void StartApplication() {
    node_.reset(new ::ndn::vsync::app::SimpleNode(
        node_id_, routing_prefix_, ndn::StackHelper::getKeyChain()));
    node_->ConnectVectorClockTrace(
        std::bind(&SimpleNodeApp::TraceVectorClock, this, _1));
    node_->ConnectViewIDTrace(std::bind(&SimpleNodeApp::TraceViewID, this, _1));
    node_->ConnectDataEventTrace(
        std::bind(&SimpleNodeApp::TraceDataEvent, this, _1, _2));
    node_->Start();
  }

  virtual void StopApplication() { node_.reset(); }

  void TraceVectorClock(const ::ndn::vsync::VersionVector& vc) {
    vector_clock_trace_(vc);
  }

  void TraceViewID(const ::ndn::vsync::ViewID& vid) { view_id_trace_(vid); }

  void TraceDataEvent(std::shared_ptr<const ndn::Data> data, bool is_local) {
    data_event_trace_(data, is_local);
  }

 private:
  std::unique_ptr<::ndn::vsync::app::SimpleNode> node_;
  std::string node_id_;
  std::string routing_prefix_;

  TracedCallback<const ::ndn::vsync::ViewID&> view_id_trace_;
  TracedCallback<const ::ndn::vsync::VersionVector&> vector_clock_trace_;
  TracedCallback<std::shared_ptr<const ndn::Data>, bool> data_event_trace_;
};

}  // namespace vsync
}  // namespace ndn
}  // namespace ns3

#endif  // SIMPLE_APP_HPP_

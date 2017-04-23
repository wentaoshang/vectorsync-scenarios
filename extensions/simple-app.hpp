/* -*- Mode:C++; c-file-style:"google"; indent-tabs-mode:nil; -*- */

#ifndef SIMPLE_APP_HPP_
#define SIMPLE_APP_HPP_

#include <limits>

#include "ns3/application.h"
#include "ns3/boolean.h"
#include "ns3/ndnSIM/helper/ndn-stack-helper.hpp"
#include "ns3/string.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/traced-callback.h"
#include "ns3/uinteger.h"

#include "simple.hpp"

namespace ns3 {
namespace ndn {
namespace vsync {

class SimpleNodeApp : public Application {
 public:
  typedef void (*VectorClockTraceCallback)(std::size_t,
                                           const ::ndn::vsync::VersionVector&);
  typedef void (*ViewChangeTraceCallback)(const ::ndn::vsync::ViewID&,
                                          const ::ndn::vsync::ViewInfo&, bool);
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
            .AddAttribute(
                "RandomSeed", "Seed used for the random number generator.",
                UintegerValue(0), MakeUintegerAccessor(&SimpleNodeApp::seed_),
                MakeUintegerChecker<uint32_t>())
            .AddAttribute("LossyMode",
                          "Whether VectorSync operates in lossy mode.",
                          BooleanValue(false),
                          MakeBooleanAccessor(&SimpleNodeApp::lossy_mode_),
                          MakeBooleanChecker())
            .AddAttribute(
                "MaxDataInterval",
                "Maximum data publishing interval for the sync node.",
                UintegerValue(10000),
                MakeUintegerAccessor(&SimpleNodeApp::max_data_interval_),
                MakeUintegerChecker<uint32_t>())
            .AddTraceSource(
                "VectorClock", "Vector clock of the sync node.",
                MakeTraceSourceAccessor(&SimpleNodeApp::vector_clock_trace_),
                "ns3::ndn::vsync::SimpleNodeApp::VectorClockTraceCallback")
            .AddTraceSource(
                "ViewChange", "View change event from the sync node.",
                MakeTraceSourceAccessor(&SimpleNodeApp::view_change_trace_),
                "ns3::ndn::vsync::SimpleNodeApp::ViewChangeTraceCallback")
            .AddTraceSource(
                "DataEvent",
                "Event of publishing or receiving new data in the sync node.",
                MakeTraceSourceAccessor(&SimpleNodeApp::data_event_trace_),
                "ns3::ndn::vsync::SimpleNodeApp::DataEventTraceCallback");

    return tid;
  }

 protected:
  virtual void StartApplication();

  virtual void StopApplication() { node_.reset(); }

  void TraceVectorClock(std::size_t idx,
                        const ::ndn::vsync::VersionVector& vc) {
    vector_clock_trace_(idx, vc);
  }

  void TraceViewChange(const ::ndn::vsync::ViewID& vid,
                       const ::ndn::vsync::ViewInfo& vinfo, bool is_leader) {
    view_change_trace_(vid, vinfo, is_leader);
  }

  void TraceDataEvent(std::shared_ptr<const ndn::Data> data, bool is_local) {
    data_event_trace_(data, is_local);
  }

 private:
  std::unique_ptr<::ndn::vsync::app::SimpleNode> node_;
  std::string node_id_;
  std::string routing_prefix_;
  uint32_t seed_;
  bool lossy_mode_;
  uint32_t max_data_interval_;

  TracedCallback<const ::ndn::vsync::ViewID&, const ::ndn::vsync::ViewInfo&,
                 bool>
      view_change_trace_;
  TracedCallback<std::size_t, const ::ndn::vsync::VersionVector&>
      vector_clock_trace_;
  TracedCallback<std::shared_ptr<const ndn::Data>, bool> data_event_trace_;
};

}  // namespace vsync
}  // namespace ndn
}  // namespace ns3

#endif  // SIMPLE_APP_HPP_

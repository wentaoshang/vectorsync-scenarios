/* -*- Mode:C++; c-file-style:"google"; indent-tabs-mode:nil; -*- */

#ifndef SIMPLE_APP_HPP_
#define SIMPLE_APP_HPP_

#include <limits>

#include "ns3/application.h"
#include "ns3/boolean.h"
#include "ns3/double.h"
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
  typedef void (*VectorChangeTraceCallback)(std::size_t,
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
            .AddAttribute("ViewInfo", "Serialized protobuf for the ViewInfo.",
                          StringValue(""),
                          MakeStringAccessor(&SimpleNodeApp::vinfo_proto_),
                          MakeStringChecker())
            .AddAttribute(
                "RandomSeed", "Seed used for the random number generator.",
                UintegerValue(0), MakeUintegerAccessor(&SimpleNodeApp::seed_),
                MakeUintegerChecker<uint32_t>())
            .AddAttribute(
                "DataRate",
                "Data publishing rate (packets per second) for the sync node.",
                DoubleValue(1.0),
                MakeDoubleAccessor(&SimpleNodeApp::data_rate_),
                MakeDoubleChecker<double>())
            .AddTraceSource(
                "VectorChange", "Vector change event from the sync node.",
                MakeTraceSourceAccessor(&SimpleNodeApp::vector_change_trace_),
                "ns3::ndn::vsync::SimpleNodeApp::VectorChangeTraceCallback")
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

  void TraceVectorChange(std::size_t idx,
                         const ::ndn::vsync::VersionVector& vc) {
    vector_change_trace_(idx, vc);
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
  uint32_t seed_;
  double data_rate_;

  std::string vinfo_proto_;

  TracedCallback<const ::ndn::vsync::ViewID&, const ::ndn::vsync::ViewInfo&,
                 bool>
      view_change_trace_;
  TracedCallback<std::size_t, const ::ndn::vsync::VersionVector&>
      vector_change_trace_;
  TracedCallback<std::shared_ptr<const ndn::Data>, bool> data_event_trace_;
};

}  // namespace vsync
}  // namespace ndn
}  // namespace ns3

#endif  // SIMPLE_APP_HPP_

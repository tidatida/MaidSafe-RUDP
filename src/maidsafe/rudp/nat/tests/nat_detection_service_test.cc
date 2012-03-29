/*******************************************************************************
 *  Copyright 2012 MaidSafe.net limited                                        *
 *                                                                             *
 *  The following source code is property of MaidSafe.net limited and is not   *
 *  meant for external use.  The use of this code is governed by the licence   *
 *  file licence.txt found in the root of this directory and also on           *
 *  www.maidsafe.net.                                                          *
 *                                                                             *
 *  You are not free to copy, amend or otherwise use this source code without  *
 *  the explicit written permission of the board of directors of MaidSafe.net. *
 ******************************************************************************/

#include "boost/thread.hpp"
#include "boost/thread/detail/thread_group.hpp"

#include "maidsafe/common/test.h"
#include "maidsafe/common/asio_service.h"

#include "maidsafe/rudp/log.h"
#include "maidsafe/rudp/nat_detection_service.h"
#include "maidsafe/rudp/transport.h"
#include "maidsafe/rudp/core/message_handler.h"
#include "maidsafe/rudp/transport_pb.h"
#include "maidsafe/rudp/nat_detection.h"

namespace args = std::placeholders;

namespace maidsafe {

namespace rudp {

namespace test {

class MockNatDetectionServiceTest;

class Node {
 public:
  Node() : asio_service_(),
           endpoint_(IP::from_string("127.0.0.1"), 0),
           live_contact_(IP::from_string("127.0.0.1"), 0),
           transport_(new transport::RudpTransport(asio_service_.service())),
           message_handler_(new RudpMessageHandler(nullptr)) {
    asio_service_.Start(5);
  }

  ~Node() {
    transport_->StopListening();
    transport_.reset();
    asio_service_.Stop();
  }

  bool StartListening() {
    ReturnCode condition(kError);
    size_t max(5), attempt(0);
    while (attempt++ < max && (condition != kSuccess)) {
      endpoint_.port = RandomUint32() % 50000 + 1025;
      condition = transport_->StartListening(endpoint_);
    }
    return (condition == kSuccess);
  }

  Endpoint endpoint() const { return endpoint_; }
  Endpoint live_contact() const { return live_contact_; }
  std::shared_ptr<RudpTransport> transport() const { return transport_; }
  RudpMessageHandlerPtr message_handler() const { return message_handler_; }
  boost::asio::io_service& io_service() { return asio_service_.service(); }

  void set_live_contact(Endpoint live_contact) { live_contact_ = live_contact; }

 private:
  AsioService asio_service_;
  Endpoint endpoint_, live_contact_;
  std::shared_ptr<RudpTransport> transport_;
  RudpMessageHandlerPtr message_handler_;
};

typedef std::shared_ptr<Node> NodePtr;

class MockNatDetectionService : public NatDetectionService {
 public:
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
#endif
  MockNatDetectionService(boost::asio::io_service &io_service,  // NOLINT
                          RudpMessageHandlerPtr message_handler,
                          RudpTransportPtr listening_transport,
                          GetEndpointFunctor get_endpoint_functor)
      : NatDetectionService(io_service, message_handler,
                            listening_transport, get_endpoint_functor) {}
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
  MOCK_METHOD2(DirectlyConnected,
               bool(const protobuf::NatDetectionRequest &request,  // NOLINT (Fraser)
                    const Endpoint &endpoint));
  MOCK_METHOD4(ConnectResult, void(const int &in_result,
                                   int *out_result,
                                   boost::mutex *mutex,
                                   boost::condition_variable* condition));
  bool NotDirectlyConnected() {
    return false;
  }

  void ConnectResultFail(int *out_result,
                         boost::mutex *mutex,
                         boost::condition_variable* condition) {
    boost::mutex::scoped_lock lock(*mutex);
    *out_result = kError;
    condition->notify_one();
  }
};

class MockNatDetectionServiceTest : public testing::Test {
 public:
  MockNatDetectionServiceTest()
      :  origin_(new Node),
         proxy_(new Node),
         rendezvous_(new Node) {}
  void ConnectToSignals(TransportPtr transport,
                        RudpMessageHandlerPtr message_handler) {
    transport->on_message_received()->connect(
          transport::OnMessageReceived::element_type::slot_type(
              &RudpMessageHandler::OnMessageReceived, message_handler.get(),
              _1, _2, _3, _4).track_foreign(message_handler));
    transport->on_error()->connect(
        transport::OnError::element_type::slot_type(
            &RudpMessageHandler::OnError,
            message_handler.get(), _1, _2).track_foreign(message_handler));
  }
  ~MockNatDetectionServiceTest() {
    proxy_.reset();
    rendezvous_.reset();
    origin_.reset();
  }

 protected:
  NodePtr origin_, proxy_, rendezvous_;
};

// TEST_F(MockNatDetectionServiceTest, BEH_FullConeDetection) {
//  NatDetection nat_detection;
//  std::shared_ptr<MockNatDetectionService> origin_service, rendezvous_service,
//       proxy_service;
//  std::vector<Contact> contacts;
//  std::vector<Endpoint> endpoints;
//  bool listens = origin_->StartListening();
//  EXPECT_TRUE(listens);
//  origin_->transport()->transport_details_.local_endpoints.push_back(
//      origin_->endpoint());
//  ConnectToSignals(origin_->transport(), origin_->message_handler());
//  listens = rendezvous_->StartListening();
//  EXPECT_TRUE(listens);
//  origin_->set_live_contact(rendezvous_->endpoint());
//  ConnectToSignals(rendezvous_->transport(), rendezvous_->message_handler());
//  listens =  proxy_->StartListening();
//  EXPECT_TRUE(listens);
//  rendezvous_->set_live_contact(proxy_->endpoint());
//  ConnectToSignals(proxy_->transport(), proxy_->message_handler());
//  origin_service.reset(
//      new MockNatDetectionService(origin_->io_service(),
//                                  origin_->message_handler(),
//                                  origin_->transport(),
//                                  std::bind(&Node::live_contact, origin_)));
//  origin_service->ConnectToSignals();
//  rendezvous_service.reset(
//      new MockNatDetectionService(rendezvous_->io_service(),
//                                  rendezvous_->message_handler(),
//                                  rendezvous_->transport(),
//                                std::bind(&Node::live_contact, rendezvous_)));
//  rendezvous_service->ConnectToSignals();
//  EXPECT_CALL(*rendezvous_service, DirectlyConnected(testing::_, testing::_))
//      .WillOnce((testing::Invoke(
//          std::bind(&MockNatDetectionService::NotDirectlyConnected,
//          rendezvous_service.get()))));
//  proxy_service.reset(
//      new MockNatDetectionService(proxy_->io_service(),
//                                  proxy_->message_handler(),
//                                  proxy_->transport(),
//                                  std::bind(&Node::live_contact, proxy_)));
//  proxy_service->ConnectToSignals();
//  NatType nattype;
//  Endpoint rendezvous_endpoint;
//  endpoints.push_back(rendezvous_->endpoint());
//  Contact rendezvous_contact(rendezvous_->endpoint(), endpoints,
//                             Endpoint(), true, true);
//  contacts.push_back(rendezvous_contact);
//  nat_detection.Detect(contacts, true, origin_->transport(),
//                       origin_->message_handler(), &nattype,
//                       &rendezvous_endpoint);
//  EXPECT_EQ(nattype, kFullCone);
// }
//
// TEST_F(MockNatDetectionServiceTest, BEH_PortRestrictedDetection) {
//  NatDetection nat_detection;
//  std::shared_ptr<MockNatDetectionService> origin_service, rendezvous_service,
//       proxy_service;
//  std::vector<Contact> contacts;
//  std::vector<Endpoint> endpoints;
//  bool listens = origin_->StartListening();
//  EXPECT_TRUE(listens);
//  origin_->transport()->transport_details_.local_endpoints.push_back(
//      origin_->endpoint());
//  ConnectToSignals(origin_->transport(), origin_->message_handler());
//  listens = rendezvous_->StartListening();
//  EXPECT_TRUE(listens);
//  origin_->set_live_contact(rendezvous_->endpoint());
//  ConnectToSignals(rendezvous_->transport(), rendezvous_->message_handler());
//  listens =  proxy_->StartListening();
//  EXPECT_TRUE(listens);
//  rendezvous_->set_live_contact(proxy_->endpoint());
//  ConnectToSignals(proxy_->transport(), proxy_->message_handler());
//  origin_service.reset(
//      new MockNatDetectionService(origin_->io_service(),
//                                  origin_->message_handler(),
//                                  origin_->transport(),
//                                  std::bind(&Node::live_contact, origin_)));
//  origin_service->ConnectToSignals();
//  rendezvous_service.reset(
//      new MockNatDetectionService(rendezvous_->io_service(),
//                                  rendezvous_->message_handler(),
//                                  rendezvous_->transport(),
//                                std::bind(&Node::live_contact, rendezvous_)));
//  rendezvous_service->ConnectToSignals();
//  EXPECT_CALL(*rendezvous_service, DirectlyConnected(testing::_, testing::_))
//      .WillOnce((testing::Invoke(
//          std::bind(&MockNatDetectionService::NotDirectlyConnected,
//          rendezvous_service.get()))));
//  proxy_service.reset(
//      new MockNatDetectionService(proxy_->io_service(),
//                                  proxy_->message_handler(),
//                                  proxy_->transport(),
//                                  std::bind(&Node::live_contact, proxy_)));
//  proxy_service->ConnectToSignals();
//  EXPECT_CALL(*proxy_service, ConnectResult(testing::_, testing::_,
//                                            testing::_, testing::_))
//       .WillOnce(testing::WithArgs<1, 2, 3>(testing::Invoke(
//           std::bind(&MockNatDetectionService::ConnectResultFail,
//           proxy_service.get(), args::_1, args::_2, args::_3))));
//  NatType nattype;
//  Endpoint rendezvous_endpoint;
//  endpoints.push_back(rendezvous_->endpoint());
//  Contact rendezvous_contact(rendezvous_->endpoint(), endpoints,
//                             Endpoint(), true, true);
//  contacts.push_back(rendezvous_contact);
//  nat_detection.Detect(contacts, true, origin_->transport(),
//                       origin_->message_handler(), &nattype,
//                       &rendezvous_endpoint);
//  EXPECT_EQ(nattype, kPortRestricted);
// }

}  // namespace test

}  // namespace rudp

}  // namespace maidsafe

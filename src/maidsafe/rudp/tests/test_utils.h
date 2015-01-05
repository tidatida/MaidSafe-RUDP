/*  Copyright 2012 MaidSafe.net limited

    This MaidSafe Software is licensed to you under (1) the MaidSafe.net Commercial License,
    version 1.0 or later, or (2) The General Public License (GPL), version 3, depending on which
    licence you accepted on initial access to the Software (the "Licences").

    By contributing code to the MaidSafe Software, or to this project generally, you agree to be
    bound by the terms of the MaidSafe Contributor Agreement, version 1.0, found in the root
    directory of this project at LICENSE, COPYING and CONTRIBUTOR respectively and also
    available at: http://www.maidsafe.net/licenses

    Unless required by applicable law or agreed to in writing, the MaidSafe Software distributed
    under the GPL Licence is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS
    OF ANY KIND, either express or implied.

    See the Licences for the specific language governing permissions and limitations relating to
    use of the MaidSafe Software.                                                                 */

#ifndef MAIDSAFE_RUDP_TESTS_TEST_UTILS_H_
#define MAIDSAFE_RUDP_TESTS_TEST_UTILS_H_

#include <cstdint>
#include <future>
#include <memory>
#include <string>
#include <vector>

#include "asio/use_future.hpp"
#include "boost/asio/ip/udp.hpp"
#include "boost/thread/future.hpp"

#include "maidsafe/common/node_id.h"
#include "maidsafe/common/rsa.h"
#include "maidsafe/common/test.h"
#include "maidsafe/common/utils.h"

#include "maidsafe/rudp/managed_connections.h"
#include "maidsafe/rudp/async_queue.h"

namespace maidsafe {

namespace rudp {

struct Contact;

typedef std::shared_ptr<ManagedConnections> ManagedConnectionsPtr;


namespace test {

class Node;
typedef std::shared_ptr<Node> NodePtr;

testing::AssertionResult SetupNetwork(std::vector<NodePtr> & nodes,
                                      std::vector<Contact> & bootstrap_endpoints, int node_count);


class Node {
 public:
  using Contacts = std::vector<Contact>;
  typedef std::vector<uint8_t>   message_t;
  typedef std::vector<message_t> messages_t;

 public:
  explicit Node(int id);
  std::vector<NodeId> connection_lost_node_ids() const;
  std::future<Contact> Bootstrap(const Contacts&, Endpoint local_endpoint = Endpoint());
  std::future<Contact> Bootstrap(Contact, Endpoint local_endpoint = Endpoint());

  std::future<EndpointPair> GetAvailableEndpoints(NodeId id) {
    return managed_connections_->GetAvailableEndpoints(id, asio::use_future);
  }

  std::future<void> Add(const Contact& contact) {
    return managed_connections_->Add(contact, asio::use_future);
  }

  std::future<void> Remove(const NodeId& id) {
    return managed_connections_->Remove(id, asio::use_future);
  }

  std::future<void> Send(const NodeId& id, const message_t& msg) {
    return managed_connections_->Send(id, msg, asio::use_future);
  }

  std::future<void> Send(const NodeId& id, const std::string& msg) {
    return managed_connections_->Send(id, message_t(msg.begin(), msg.end()), asio::use_future);
  }

  std::future<message_t> Receive() { return message_queue_.async_pop(asio::use_future); }

  std::string id() const { return id_; }
  NodeId node_id() const { return node_id_; }
  std::string debug_node_id() const { return DebugId(node_id_); }

  Contact make_contact(EndpointPair eps) const { return Contact{node_id_, eps, public_key()}; }

  asymm::Keys keys() const { return key_pair_; }

  asymm::PrivateKey private_key() const { return key_pair_.private_key; }
  asymm::PublicKey  public_key()  const { return key_pair_.public_key; }

  ManagedConnectionsPtr managed_connections() const { return managed_connections_; }

  int GetReceivedMessageCount(const message_t& message) const;

  void ResetData();

  std::vector<NodeId> GetConnectedNodeIds() { return connected_node_ids_; }

  void AddConnectedNodeId(const NodeId& connected_node_id) {
    connected_node_ids_.push_back(connected_node_id);
  }

  static message_t str_to_msg(const std::string& str) {
    return message_t(str.begin(), str.end());
  }

  size_t GetActiveConnectionCount() const {
    return managed_connections_->GetActiveConnectionCount();
  }

  async_queue<std::error_code, message_t>& message_queue() { return message_queue_; }

 private:
  NodeId node_id_;
  std::string id_;
  asymm::Keys key_pair_;
  mutable std::mutex mutex_;
  std::vector<NodeId> connection_lost_node_ids_;
  std::vector<NodeId> connected_node_ids_;
  messages_t messages_;
  async_queue<std::error_code, message_t> message_queue_;
  ManagedConnectionsPtr managed_connections_;

  std::shared_ptr<ManagedConnections::Listener> bootstrap_listener_;
};


}  // namespace test

}  // namespace rudp

}  // namespace maidsafe

#endif  // MAIDSAFE_RUDP_TESTS_TEST_UTILS_H_

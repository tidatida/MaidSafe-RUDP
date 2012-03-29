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

#ifndef MAIDSAFE_RUDP_PARAMETERS_H_
#define MAIDSAFE_RUDP_PARAMETERS_H_

#include <cassert>
#include <deque>

#include "boost/cstdint.hpp"
#include "maidsafe/common/utils.h"
#include "maidsafe/rudp/version.h"

#if MAIDSAFE_RUDP_VERSION != 100
#  error This API is not compatible with the installed library.\
    Please update the maidsafe_rudp library.
#endif

namespace maidsafe {

namespace rudp {

// This class provides the configurability to all traffic related parameters.
struct Parameters {
 public:
  // Window size permitted in RUDP
  static boost::uint32_t default_window_size;
  static boost::uint32_t maximum_window_size;

  // Packet size permitted in RUDP
  // Shall not exceed the UDP payload, which is 65507
  static boost::uint32_t default_size;
  static boost::uint32_t max_size;
  enum { kUDPPayload = 65500 };

  // Data Payload size permitted in RUDP
  // Shall not exceed Packet Size defined
  static boost::uint32_t default_data_size;
  static boost::uint32_t max_data_size;

  // Timeout defined for a packet to be resent
  static boost::posix_time::time_duration default_send_timeout;

  // Timeout defined for a neg-ack packet to be resent to request resent of an
  // observed missing packet in receiver
  static boost::posix_time::time_duration default_receive_timeout;

  // Machine dependent parameter of send delay,
  // depending on computation power and I/O speed
  static boost::posix_time::time_duration default_send_delay;

  // Machine dependent parameter of receive delay,
  // depending on computation power and I/O speed
  static boost::posix_time::time_duration default_receive_delay;

  // Timeout defined for a Ack packet to be resent
  static boost::posix_time::time_duration default_ack_timeout;

  // Timeout defined the fixed interval between Ack packets
  static boost::posix_time::time_duration ack_interval;

  // Interval to calculate speed
  static boost::posix_time::time_duration speed_calculate_inverval;

  // Slow speed threshold to force the socket closed, in b/s
  static boost::uint32_t slow_speed_threshold;

  // Timeout during client connection establishment
  static boost::posix_time::time_duration client_connect_timeout;

  // Defined connection types
  enum ConnectionType {
    kWireless = 0x0fffffff,
    kT1 = 0xf0ffffff,
    kE1 = 0xf1ffffff,
    k10MEthernet = 0xff0fffff,
    k100MEthernet = 0xff1fffff,
    k1GEthernet = 0xff2fffff
  };
  static ConnectionType connection_type;

 private:
  // Disallow copying and assignment.
  Parameters(const Parameters&);
  Parameters &operator=(const Parameters&);
};

}  // namespace rudp

}  // namespace maidsafe

#endif  // MAIDSAFE_RUDP_PARAMETERS_H_

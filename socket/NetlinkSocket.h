#ifndef PW_NETLINK_SOCKET_H
#define PW_NETLINK_SOCKET_H
#pragma once

#include "socket/Socket.h"

#include <linux/netlink.h>
#include <linux/cn_proc.h>
#include <linux/connector.h>

namespace pw_socket {

namespace netlink {
/**
 * @brief Represents a message structure used for enabling or disabling
 * proc events from a netlink connection
 */
struct __attribute__ ((aligned(NLMSG_ALIGNTO))) NetlinkListenMessage {
  struct nlmsghdr hdr;
  struct __attribute__ ((__packed__)) {
    struct cn_msg msg;
    enum proc_cn_mcast_op mcast;
  };
};

/**
 * @brief Represents a message structure used for reading
 * proc events from a netlink connection
 */
struct __attribute__ ((aligned(NLMSG_ALIGNTO))) NetlinkEventMessage {
  nlmsghdr header;
  struct __attribute__ ((__packed__)) {
    cn_msg msg;
    proc_event proc_ev;
  };
};
 
} // end namespace netlink

/**
 * @brief Class representing a linux netlink socket, providing auto close on destruction
 * @note Class is not copyable
 * @author Nik Twerdochlib
 * @since Sun Oct 03 2021
 */
class NetlinkSocket
  : public Socket
{
public:
  NetlinkSocket();
  NetlinkSocket(const NetlinkSocket&) = delete;
  NetlinkSocket(NetlinkSocket&&) = default;

  ~NetlinkSocket() override;

  NetlinkSocket& operator=(const NetlinkSocket&) = delete;
  NetlinkSocket& operator=(NetlinkSocket&&) = default;

  /**
   * @copydoc Socket::create
   */
  bool create() override;
  /**
   * @copydoc Socket::connect
   */
  bool connect() override;
  /**
   * @copydoc Socket::send
   */
  bool send(const void* buffer, std::size_t bytes, std::size_t* bytesSent = nullptr) override;
  /**
   * @copydoc Socket::recv
   */
  bool recv(void* buffer, std::size_t bytes, std::size_t* bytesRcvd = nullptr) override;

  /**
   * @brief Enables/Disables listening for proc events on the undelying netlink connection
   * @param enable - true for enabling events, false for disabling events
   * @return true on success, false on failure
   */
  bool listenForEvents(bool enable);
};

} // end namespace pw_socket
#endif // PW_NETLINK_SOCKET_H
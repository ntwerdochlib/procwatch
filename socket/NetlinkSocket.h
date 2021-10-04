#ifndef PW_NETLINK_SOCKET_H
#define PW_NETLINK_SOCKET_H
#pragma once

#include "socket/Socket.h"

#include <linux/netlink.h>
#include <linux/cn_proc.h>
#include <linux/connector.h>

namespace pw_socket {

namespace netlink {
struct __attribute__ ((aligned(NLMSG_ALIGNTO))) NetlinkListenMessage {
  struct nlmsghdr hdr;
  struct __attribute__ ((__packed__)) {
    struct cn_msg msg;
    enum proc_cn_mcast_op mcast;
  };
};

struct __attribute__ ((aligned(NLMSG_ALIGNTO))) NetlinkEventMessage {
  nlmsghdr header;
  struct __attribute__ ((__packed__)) {
    cn_msg msg;
    proc_event proc_ev;
  };
};
 
} // end namespace netlink

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

  bool create() override;
  bool connect() override;
  bool send(const void* buffer, std::size_t bytes, std::size_t* bytesSent = nullptr) override;

  bool listenForEvents(bool enable);
};

} // end namespace pw_socket
#endif // PW_NETLINK_SOCKET_H
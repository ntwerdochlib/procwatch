#include "socket/NetlinkSocket.h"

#include <sys/socket.h>
#include <linux/connector.h>
#include <err.h>
#include <string.h>

#include <iostream>

namespace pw_socket {

NetlinkSocket::NetlinkSocket()
  : Socket()
{}

NetlinkSocket::~NetlinkSocket()
{}

bool NetlinkSocket::create()
{
  m_handle = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_CONNECTOR);
  if (m_handle == -1) {
    err(errno, "Failed creating netlink socket");
    return false;
  }

	return true;
}

bool NetlinkSocket::connect()
{
  sockaddr_nl sa {
    .nl_family = AF_NETLINK,
    .nl_pid = static_cast<__u32>(pthread_self() << 16 | getpid()),
    .nl_groups = CN_IDX_PROC,
  };

  auto const r = bind(m_handle, reinterpret_cast<sockaddr *>(&sa), sizeof(sa));
  if (r == -1) {
    err(errno, "Failed binding to netlink socket");
    return false;
  }

  return true;
}

bool NetlinkSocket::send(const void* buffer, std::size_t bytes, std::size_t* bytesSent)
{
  auto const r = ::send(m_handle, buffer, bytes, 0);
  if (bytesSent) {
    *bytesSent = r;
  }
  if (r <= 0) {
    err(errno, "Failed sending %d bytes on netlink socket", bytes);
    return false;
  }

  return true;
}

bool NetlinkSocket::recv(void* buffer, std::size_t bytes, std::size_t* bytesRcvd)
{
  auto const r = ::recv(m_handle, buffer, bytes, 0);
  if (bytesRcvd) {
    *bytesRcvd = r;
  }
  if (r <= 0) {
    err(errno, "Failed recieving data on netlink socket");
    return false;
  }

  return true;
}

bool NetlinkSocket::listenForEvents(bool enable)
{
  pw_socket::netlink::NetlinkListenMessage listenRequest {0};

  listenRequest.hdr.nlmsg_len = sizeof(listenRequest);
  listenRequest.hdr.nlmsg_type = NLMSG_DONE;
  listenRequest.hdr.nlmsg_pid = static_cast<__u32>(getpid());
  listenRequest.msg.id.idx = CN_IDX_PROC;
  listenRequest.msg.id.val = CN_VAL_PROC;
  listenRequest.msg.len = sizeof (proc_cn_mcast_op);
  listenRequest.mcast = (enable ? PROC_CN_MCAST_LISTEN : PROC_CN_MCAST_IGNORE);
  return send(&listenRequest, sizeof(listenRequest));
}

} // end namespace pw_socket
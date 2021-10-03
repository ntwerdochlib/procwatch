#ifndef PW_CLIENT_H
#define PW_CLIENT_H
#pragma once

#include "socket/NetlinkSocket.h"
#include "socket/UnixSocket.h"
#include "util/handle.h"

class ProcWatchClient
{
  public:
  ProcWatchClient(const pw_socket::UnixSocket& socket);
  ~ProcWatchClient();

  bool run();

private:
  void initialize();

private:
  pw_socket::UnixSocket m_socket;
  pw_socket::NetlinkSocket m_netlink;
  pw_util::Handle m_epoll;
}
#endif // PW_CLIENT_H
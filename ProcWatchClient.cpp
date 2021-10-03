#include "ProcWatchClient.h"

ProcWatchClient::ProcWatchClient(const pw_socket::UnixSocket& socket)
  : m_socket(socket)
{
  initialize();
}

ProcWatchClient::~ProcWatchClient()
{}

void ProcWatchClient::initialize()
{
  if (!m_netlink.create()) {
    throw std::runtime_error("failed creating netlink socket");
  }

  if (!m_netlink.connect()) {
    throw std::runtime_error("failed connecting netlink socket");
  }

  // Request events from netllink
  if (!m_netlink.listenForEvents(true)) {
    throw std::runtime_error("failed to configure listen for events on netlink socket");
  }
}

bool ProcWatchClient::run()
{}
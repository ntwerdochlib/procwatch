#include "ProcWatchSvr.h"

#include <iostream>
#include <iomanip>
#include <list>
#include <sstream>

#include <err.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>

#include "util/utils.h"
#include "util/handle.h"

#include "config.h"
#include "socket/NetlinkSocket.h"

void ProcWatchSvr::run(std::stop_token stopToken)
{
  // Establish the server socket, using a hidden socket
  pw_socket::UnixSocket serverSocket{pw_config::ServerEndpoint};

  if (!serverSocket.create()) {
    throw std::runtime_error("failed to create server socket");
  }

  if (!serverSocket.listen(true)) {
    throw std::runtime_error("failed listening on socket");
  }

  // Setup epoll
  auto epollHandle = pw_util::CreateEpoll(2);
  pw_util::AddHandleToEpoll(epollHandle, serverSocket.handle(), EPOLLIN);

  // Setup the netlink connection
  pw_socket::NetlinkSocket netlink;
  if (!netlink.create()) {
    throw std::runtime_error("failed creating netlink socket");
  }

  if (!netlink.connect()) {
    throw std::runtime_error("failed connecting netlink socket");
  }

  // Add the netlink connection to epoll, but for hangup only.  EPOLLIN will be added
  // once we have a client connection. Until then we don't want to waste resources
  pw_util::AddHandleToEpoll(epollHandle, netlink, EPOLLHUP);

  std::cout << __func__ << ": Running\n";

  // Represents the sole client connection allowed
  std::shared_ptr<pw_socket::UnixSocket> clientSocket;

  auto handleClientClose = [&epollHandle, &netlink](std::shared_ptr<pw_socket::UnixSocket>& clientSocket) {
    // No longer interested in netlink proc events
    pw_util::ModifyHandleInEpoll(epollHandle, netlink, EPOLLHUP);
    if (!netlink.listenForEvents(false)) {
      std::cerr << "Failed to configure listen for event on netlink\n";
    }
    // Release the client to rest in peace, on the beautiful country hill side...
    clientSocket.reset();
  };

  //TODO(nik): Determine what the optimal poll size really is for this use case
  static constexpr int EpollSize = 2;
  epoll_event events[EpollSize]{0};

  // Loop until we get a stop request
  while (!stopToken.stop_requested()) {
    auto const fdCount = epoll_wait(epollHandle, events, EpollSize, 500);
    if (fdCount == 0) {
      continue;
    } else if (fdCount < 0) {
      err(errno, "Error polling sockets");
      break;
    }
    for (int i = 0; i < fdCount; ++i) {
      auto const& ev = events[i];
      if (ev.data.fd == serverSocket) {
        // Process server connection request
        try {
          std::cout << "Server connection. Events: " << std::hex << ev.events << std::endl;
          auto client = serverSocket.acceptConnection();
          // If we have already met out connection quota then inform the client and disconnect
          if (clientSocket) {
            client->send("Too many connections");
            continue;
          }
          std::cout << "Client connected\n";
          // We only want to know if the client closes the connection
          pw_util::AddHandleToEpoll(epollHandle, client->handle(), EPOLLHUP);
          // Now we are ready to receive proc events from the netlink connection
          if (!netlink.listenForEvents(true)) {
            throw std::runtime_error("failed to configure listen for event on netlink");
          }
          pw_util::ModifyHandleInEpoll(epollHandle, netlink, EPOLLIN);
          // This is now our one and only client connection
          clientSocket = std::move(client);
        } catch (std::exception const& e) {
          std::cerr << "Exception accepting client connection: " << e.what() << std::endl;
        }
      } else if (ev.data.fd == netlink) {
        // Handle proc events from netlink
        if (ev.events == EPOLLIN && clientSocket) {
          pw_socket::netlink::NetlinkEventMessage nlcn_msg{};
          auto const r = netlink.recv(&nlcn_msg, sizeof(nlcn_msg));
          if (r == 0) {
            break;
          } else if (r == -1) {
            if (errno == EINTR) {
              continue;
            }
            err(errno, "Read error on netlink socket");
            break;
          }

          if (nlcn_msg.header.nlmsg_type == NLMSG_ERROR || nlcn_msg.header.nlmsg_type == NLMSG_NOOP) {
            continue;
          }

          try {
            // Process the netlink connection message, sending any requried information to the client
            handleNetlinkEvent(nlcn_msg.proc_ev, clientSocket);
          } catch (std::exception const& e) {
            std::cout << "Exception sending message to client, disconnecting. " << e.what();
            handleClientClose(clientSocket);
          }
        }
      } else {
        // The client connection has closed
        std::cout << "handle client request. events: " << std::hex << ev.events << std::endl;
        handleClientClose(clientSocket);
        std::cout << __func__ << ":Good bye dear client\n";
      }
    }
  }
  std::cout << __func__ << ": Finished\n";
}

void ProcWatchSvr::handleNetlinkEvent(const proc_event& ev, const client_t& client)
{
  std::ostringstream os;
  switch (ev.what) {
  case proc_event::what::PROC_EVENT_NONE:
    std::cout << "set mcast listen ok" << std::endl;
    break;
  case proc_event::what::PROC_EVENT_FORK:
    // Why oh why is std::format at the bottom of the list of features to add to g++ for c++20???
    std::cout << "fork: parent tid=" << std::dec << ev.event_data.fork.parent_tgid << " pid=" << ev.event_data.fork.parent_pid << " -> child tid=" << ev.event_data.fork.child_tgid << " pid=" << ev.event_data.fork.child_pid <<
      " Path: " << getProcBinaryPath(ev.event_data.fork.child_pid) << " Command Line: " << getProcCommandLine(ev.event_data.fork.child_pid) << std::endl;
    os << std::setw(8) << ev.event_data.fork.child_pid << " : " << getProcBinaryPath(ev.event_data.fork.child_pid) << " : " << getProcCommandLine(ev.event_data.fork.child_pid) << std::endl;
    break;
  case proc_event::what::PROC_EVENT_EXEC:
    std::cout << "exec: tid=" << std::dec << ev.event_data.exec.process_tgid << " pid=" << ev.event_data.exec.process_pid <<
      " Path: " << getProcBinaryPath(ev.event_data.exec.process_pid) << " Command Line: " << getProcCommandLine(ev.event_data.exec.process_pid) << std::endl;
    os << std::setw(8) << ev.event_data.exec.process_pid << " : " << getProcBinaryPath(ev.event_data.exec.process_pid) << " : " << getProcCommandLine(ev.event_data.exec.process_pid) << std::endl;
    break;
  case proc_event::what::PROC_EVENT_UID:
    std::cout << "uid change: tid=" << ev.event_data.id.process_tgid << " pid=" << ev.event_data.id.process_pid << " from " << ev.event_data.id.r.ruid << " to " << ev.event_data.id.e.euid << std::endl;
    break;
  case proc_event::what::PROC_EVENT_GID:
    std::cout << "gid change: tid=" << ev.event_data.id.process_tgid << " pid=" << ev.event_data.id.process_pid << " from " << ev.event_data.id.r.rgid << " to" << ev.event_data.id.e.egid << std::endl;
    break;
  case proc_event::what::PROC_EVENT_EXIT:
    std::cout << "exit: tid=" << ev.event_data.exit.process_tgid << " pid=" << ev.event_data.exit.process_tgid << " exit_code=" << ev.event_data.exit.exit_code << std::endl;
    break;
  case proc_event::what::PROC_EVENT_COMM:
    std::cout << "comm: tid=" << ev.event_data.comm.process_tgid << " pid=" << ev.event_data.comm.process_pid << std::endl;
    break;
  case proc_event::what::PROC_EVENT_PTRACE:
    std::cout << "ptrace: tid=" << ev.event_data.ptrace.process_tgid << " pid=" << ev.event_data.ptrace.process_pid << " tracer tid=" << ev.event_data.ptrace.tracer_tgid << " tracer pid=" << ev.event_data.ptrace.tracer_pid << std::endl;
    break;
  case proc_event::what::PROC_EVENT_SID:
    std::cout << "sid: tid=" << ev.event_data.comm.process_tgid << " pid=" <<  ev.event_data.comm.process_pid << std::endl;
    break;
  default:
    std::cerr << "unhandled proc event: " << std::hex << ev.what;
    break;
  }
  auto const message = os.str();
  if (!message.empty()) {
    // send the message to the client, fragmenting the data into pw_config::MaxMessageSize chunks
    for (size_t idx = 0; idx < message.size();) {
      int fragmentSize = (message.size() - idx > pw_config::MaxMessageSize ? pw_config::MaxMessageSize : message.size() - idx);
      size_t bytesSent = 0;
      if (!client->send(message.c_str() + idx, fragmentSize, &bytesSent)) {
        if (bytesSent < 1) {
          // error occured
          throw std::runtime_error("failed sending message to client");
        }
      }
      // only adjust idx by the bytes actually sent
      idx += bytesSent;
    }
  }
}

std::string ProcWatchSvr::getProcBinaryPath(uint32_t pid) const
{
  std::ostringstream os;
  os << "/proc/" << pid << "/exe";
  std::array<char, 1024 * 4> buffer{};
  auto r = readlink(os.str().c_str(), buffer.data(), buffer.size());
  if (r < 0) {
    return "[Unable to get binary path]";
  }
  return std::string(buffer.data(), r);
}

std::string ProcWatchSvr::getProcCommandLine(uint32_t pid) const
{
  std::ostringstream os;
  os << "/proc/" << pid << "/cmdline";
  auto h = pw_util::Handle(open(os.str().c_str(), O_RDONLY));
  if (h < 0) {
    os.clear();
    os << "/proc/" << pid << "/comm";
    h = pw_util::Handle(open(os.str().c_str(), O_RDONLY));
    if (h < 0) {
      return "[Unable To Retrieve Command Line]";
    }
  }
  //TODO(nik): Determine if a 4k buffer is adequate or actually overkill
  std::array<char, 1024 * 4> buffer{};
  auto r = read(h, buffer.data(), buffer.size());
  if (r < 0) {
    return "[Unable To Retrieve Command Line]";
  }
  // The cmdline contians a multi-string (strings delimited by a null char). Convert all nulls to spaces
  std::replace(begin(buffer), end(buffer), 0x00, 0x20);
  // if the string ends with a space, reduce the length by 1 so we ignore it
  if (buffer[r-1] == 0x20) {
    --r;
  }
  return std::string(buffer.data(), r);
}
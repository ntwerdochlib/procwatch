#include "ProcWatchSvr.h"

#include <err.h>
#include <signal.h>
#include <sys/signalfd.h>
#include <unistd.h>

#include <deque>
#include <functional>
#include <iostream>

#include "util/utils.h"
#include "util/handle.h"
#include "socket/NetlinkSocket.h"

struct Event
{
  proc_event event;
  std::string message;
};

std::deque<Event> gEventQueue;

void ProcessEventQueue(std::stop_token stopToken, const std::list<ProcWatchSvr::client_t>& clients)
{
  while (!stopToken.stop_requested()) {
    if (!gEventQueue.empty()) {
      auto event = gEventQueue.front();
      for (auto const client : clients) {
        client->send(event.message.c_str(), event.message.size());
      }
      gEventQueue.pop_front();
    } else {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  }
}

int main(int argc, char* argv[])
{
  try {
    auto epollHandle = pw_util::CreateEpoll();
    auto signalHandle = pw_util::CreateSignalFd({SIGHUP,SIGTERM,SIGINT,SIGQUIT});
    pw_util::AddHandleToEpoll(epollHandle, signalHandle, EPOLLIN);

    pw_socket::NetlinkSocket nls;

    if (!nls.create()) {
      throw std::runtime_error("failed creating netlink socket");
    }

    if (!nls.connect()) {
      throw std::runtime_error("failed connecting netlink socket");
    }

    // Request events from netllink
    if (!nls.listenForEvents(true)) {
      throw std::runtime_error("failed to configure listen for events on netlink socket");
    }

    pw_util::AddHandleToEpoll(epollHandle, nls, EPOLLIN);

    ProcWatchSvr server;
    std::jthread eventQueueThread(ProcessEventQueue, server.clients());
    server.start();

    bool running = true;
    epoll_event ev{};
    while (running && epoll_wait(epollHandle, &ev, 1, -1) > 0) {
      if (ev.data.fd == signalHandle) {
        std::cerr << __func__ << ": Got signal\n";
        signalfd_siginfo si{};
        if (read(signalHandle, &si, sizeof(si)) != sizeof(si)) {
          std::cerr << "failed to read recieved signal info\n";
          running = false;
          break;
        }
        switch (si.ssi_signo) {
          case SIGHUP:
          case SIGTERM:
          case SIGINT:
          case SIGQUIT:
            std::cout << __func__ << ": Got signal, shutting down\n";
            running = false;
            break;
          default:
            break;
        }
      } else if (ev.data.fd == nls) {
        pw_socket::netlink::NetlinkEventMessage nlcn_msg{};
        auto const rc = nls.recv(&nlcn_msg, sizeof(nlcn_msg));
        if (rc == 0) {
          /* shutdown? */
          return 0;
        } else if (rc == -1) {
          if (errno == EINTR) continue;
          perror("netlink recv");
          return -1;
        }

        gEventQueue.emplace_back(Event{nlcn_msg.proc_ev});

        switch (nlcn_msg.proc_ev.what) {
        case proc_event::what::PROC_EVENT_NONE:
          printf("set mcast listen ok\n");
          break;
        case proc_event::what::PROC_EVENT_FORK:
          printf("fork: parent tid=%d pid=%d -> child tid=%d pid=%d\n",
              nlcn_msg.proc_ev.event_data.fork.parent_pid,
              nlcn_msg.proc_ev.event_data.fork.parent_tgid,
              nlcn_msg.proc_ev.event_data.fork.child_pid,
              nlcn_msg.proc_ev.event_data.fork.child_tgid);
          break;
        case proc_event::what::PROC_EVENT_EXEC:
          printf("exec: tid=%d pid=%d\n",
              nlcn_msg.proc_ev.event_data.exec.process_pid,
              nlcn_msg.proc_ev.event_data.exec.process_tgid);
          break;
        case proc_event::what::PROC_EVENT_UID:
          printf("uid change: tid=%d pid=%d from %d to %d\n",
              nlcn_msg.proc_ev.event_data.id.process_pid,
              nlcn_msg.proc_ev.event_data.id.process_tgid,
              nlcn_msg.proc_ev.event_data.id.r.ruid,
              nlcn_msg.proc_ev.event_data.id.e.euid);
          break;
        case proc_event::what::PROC_EVENT_GID:
          printf("gid change: tid=%d pid=%d from %d to %d\n",
              nlcn_msg.proc_ev.event_data.id.process_pid,
              nlcn_msg.proc_ev.event_data.id.process_tgid,
              nlcn_msg.proc_ev.event_data.id.r.rgid,
              nlcn_msg.proc_ev.event_data.id.e.egid);
          break;
        case proc_event::what::PROC_EVENT_EXIT:
          printf("exit: tid=%d pid=%d exit_code=%d\n",
              nlcn_msg.proc_ev.event_data.exit.process_pid,
              nlcn_msg.proc_ev.event_data.exit.process_tgid,
              nlcn_msg.proc_ev.event_data.exit.exit_code);
          break;
        case proc_event::what::PROC_EVENT_COMM:
          printf("comm: tid=%d pid=%d\n",
              nlcn_msg.proc_ev.event_data.comm.process_pid,
              nlcn_msg.proc_ev.event_data.comm.process_tgid
              );
          break;
        case proc_event::what::PROC_EVENT_PTRACE:
          printf("ptrace: tid=%d pid=%d tracer tid=%d tracer pid=%d\n",
              nlcn_msg.proc_ev.event_data.ptrace.process_pid,
              nlcn_msg.proc_ev.event_data.ptrace.process_tgid,
              nlcn_msg.proc_ev.event_data.ptrace.tracer_pid,
              nlcn_msg.proc_ev.event_data.ptrace.tracer_tgid
              );
          break;
        case proc_event::what::PROC_EVENT_SID:
          printf("sid: tid=%d pid=%d\n",
              nlcn_msg.proc_ev.event_data.comm.process_pid,
              nlcn_msg.proc_ev.event_data.comm.process_tgid
              );
          break;
        default:
          printf("unhandled proc event: %X\n", nlcn_msg.proc_ev.what);
          break;
        }
      }
    }

    return 0;
  } catch (std::exception const& e) {
    std::cerr << "Exception: " << e.what() << std::endl;
    return -9;
  }
}
/**
 * @brief procwatch server
 * Allows for a single client connection from a procwatch client and send details
 * for all new processes created for the duration of the client connection.
 */
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

int main(int argc, char* argv[])
{
  try {
    auto epollHandle = pw_util::CreateEpoll();
    auto signalHandle = pw_util::CreateSignalFd({SIGHUP,SIGTERM,SIGINT,SIGQUIT});
    pw_util::AddHandleToEpoll(epollHandle, signalHandle, EPOLLIN);

    ProcWatchSvr server;
    // Run the ProcWatchSvr in separate thread
    std::jthread serverThread(std::bind(&ProcWatchSvr::run, &server, std::placeholders::_1));

    // block waiting for a singal to exit
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
      }
    }
    return 0;
  } catch (std::exception const& e) {
    std::cerr << "Exception: " << e.what() << std::endl;
    return -9;
  }
}
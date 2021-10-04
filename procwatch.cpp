#include "socket/UnixSocket.h"

#include <iostream>

#include "util/utils.h"
#include "config.h"

int main(int argc, char* argv[])
{
  try {
    pw_socket::UnixSocket socket(pw_config::ServerEndpoint);
    if (!socket.create()) {
      throw std::runtime_error("failed to create socket");
    }

    if (!socket.connect()) {
      throw std::runtime_error("failed to connect to server");
    }

    auto signalHandle = pw_util::CreateSignalFd({SIGHUP,SIGTERM,SIGINT,SIGQUIT});
    auto epollHandle = pw_util::CreateEpoll();

    pw_util::AddHandleToEpoll(epollHandle, signalHandle, EPOLLIN);
    pw_util::AddHandleToEpoll(epollHandle, socket, EPOLLIN);

    bool running = true;
    epoll_event ev{};
    std::cout << "Running...\n";
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
      } else if (ev.data.fd == socket) {
        std::array<char, 256> buffer;
        socket.recv(buffer.data(), buffer.size());
        std::cout << "Server sent: " << buffer.data() << std::endl;
        running = false;
      }
    }

    return 0;
  } catch (std::exception const& e) {
    std::cerr << "Exception: " << e.what() << std::endl;
    return -9;
  }
  std::cout << "Finished\n";
  return 0;
}
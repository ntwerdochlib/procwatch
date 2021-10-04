/**
 * @brief procwatch client
 * Connects to the procwatch server and displays information recieved regarding new process
 * created through the lifespan of the connection
 */
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
        std::cerr << __func__ << ": Got signal, exiting\n";
        break;
      } else if (ev.data.fd == socket) {
        std::array<char, pw_config::MaxMessageSize + 1> buffer{0};
        size_t bytesRcvd{0};
        if (!socket.recv(buffer.data(), buffer.size(), &bytesRcvd)) {
          if (bytesRcvd < 1) {
            throw std::runtime_error("recieve error");
          }
        }
        buffer[bytesRcvd] = 0x00;
        std::cout << buffer.data();
        if (ev.events & EPOLLHUP) {
          std::cout << "Client exiting\n";
          running = false;
          continue;
        }
      }
    }

    std::cout << "Finished\n";
    return 0;
  } catch (std::exception const& e) {
    std::cerr << "Exception: " << e.what() << std::endl;
    return -9;
  }
}
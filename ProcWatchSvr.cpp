#include "ProcWatchSvr.h"

#include <iostream>
#include <list>

#include <err.h>
#include <sys/epoll.h>
#include <unistd.h>

#include "util/utils.h"
#include "util/handle.h"

ProcWatchSvr::ProcWatchSvr()
{
}

ProcWatchSvr::~ProcWatchSvr()
{
  std::cerr << __func__ << std::endl;
  shutdown();
  if (m_thread.joinable()) {
    m_thread.join();
  }
}

void ProcWatchSvr::start()
{
  m_thread = std::move(std::jthread(std::bind(&ProcWatchSvr::run, this, std::placeholders::_1)));
}

void ProcWatchSvr::shutdown()
{
  std::cerr << __func__ << std::endl;
  m_thread.request_stop();
}

void ProcWatchSvr::run(std::stop_token stopToken)
{
  std::stop_callback shutdown(stopToken, []{ alarm(1);});

  pw_socket::UnixSocket serverSocket{"procwatch.socket"};

  if (!serverSocket.create()) {
    std::cerr << "Failed to create socket\n";
    return;
  }

  if (!serverSocket.listen()) {
    std::cerr << "Failed listening on socket\n";
    return;
  }

  auto signalHandle = pw_util::CreateSignalFd({SIGALRM});
  auto epollHandle = pw_util::CreateEpoll(1);
  pw_util::AddHandleToEpoll(epollHandle, serverSocket.handle(), EPOLLIN);
  pw_util::AddHandleToEpoll(epollHandle, signalHandle, EPOLLIN);

  std::cout << __func__ << ": Running\n";
  epoll_event ev{0};
  while (!stopToken.stop_requested()) {
    if (epoll_wait(epollHandle, &ev, 1, -1) <= 0) {
      break;
    }
    if (ev.data.fd == signalHandle) {
      // This handle will only be signalled for SIGALRM so there is no
      // need to perform any processing.  Since SIGALRM is only used currently
      // to trigger epoll_wait on a stop request, we don't even waste time
      // reading it off the signal fd
      std::cerr << __func__ << ": Got signal\n";
    } else if (ev.data.fd == serverSocket) {
      try {
        std::cout << "Server connection. Events: " << std::hex << ev.events << std::endl;
        auto client = pw_socket::UnixSocket::acceptConnection(ev.data.fd);
        std::cout << "Client connected\n";
        pw_util::AddHandleToEpoll(epollHandle, client->handle(), EPOLLHUP);
        if (!client->send("Test message from server", 24)) {
          std::cerr << "Failed to send test message to client\n";
        }
        m_clients.emplace_back(client);
      } catch (std::exception const& e) {
        std::cerr << "Exception accepting client connection: " << e.what() << std::endl;
      }
    } else {
      std::cout << "handle client request. events: " << std::hex << ev.events << std::endl;
      m_clients.remove_if([fd=ev.data.fd](auto client) { return client->handle() == fd; });
      std::cout << "Good bye\n";
    }
  }
  std::cout << __func__ << ": Finished\n";
}
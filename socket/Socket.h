#ifndef SOCKET_H
#define SOCKET_H
#pragma once

#include <unistd.h>

#include <cstddef>
#include <iostream>

namespace pw_socket {
static constexpr int InvalidSocketHandle {-1};

class Socket {
 public:
  Socket() = default;
  Socket(const Socket&) = delete;
  virtual ~Socket() {
    std::cout << __func__ << ": handle: " << m_handle << std::endl;
    (void)close();
  }

  Socket& operator=(const Socket&) = delete;
  virtual bool create() = 0;
  virtual bool connect() = 0;

  virtual bool listen() {
    return false;
  }

  virtual bool send(const void* buffer, std::size_t bytes, std::size_t* bytesWrote = nullptr) = 0;
  virtual bool recv(void* buffer, std::size_t bytes, std::size_t* bytesRcvd = nullptr) = 0;

  virtual bool close() {
    auto r {true};
    if (m_handle != InvalidSocketHandle) {
      r = (::close(m_handle) != -1 ? true : false);
      m_handle = -1;
    }
    return r;
  }

  bool operator==(int handle) const {
    return m_handle == handle;
  }

  operator int () {
    return handle();
  }

  const int& handle() const { return m_handle; }

protected:
  explicit Socket(int handle) : m_handle(handle) {}

protected:
  int m_handle{-1};
};

} // end namespace pw_socket

#endif // SOCKET_H
#ifndef SOCKET_H
#define SOCKET_H
#pragma once

#include <unistd.h>

#include <cstddef>
#include <iostream>

namespace pw_socket {
static constexpr int InvalidSocketHandle {-1};

/**
 * @brief Class representing a basic unix socket, providing auto close on destruction
 * @note Class is not copyable
 * @author Nik Twerdochlib
 * @since Sun Oct 03 2021
 */
class Socket {
 public:
  Socket() = default;
  Socket(const Socket&) = delete;
  virtual ~Socket() {
    (void)close();
  }

  Socket& operator=(const Socket&) = delete;

  /**
   * @brief Override to implement socket creation functionality
   * @return true on success, false on failure
   */
  virtual bool create() = 0;

  /**
   * @brief Override to implement socket connect functionality
   * @return true on success, false on failure
   */
  virtual bool connect() = 0;

  /**
   * @brief Override to implement socket send functionality
   * @return true on success, false on failure
   */
  virtual bool send(const void* buffer, std::size_t bytes, std::size_t* bytesWrote = nullptr) = 0;
  /**
   * @brief Override to implement socket recieve functionality
   * @return true on success, false on failure
   */
  virtual bool recv(void* buffer, std::size_t bytes, std::size_t* bytesRcvd = nullptr) = 0;

  /**
   * @brief Closees the underlying socket
   * @return true on success, false on failure
   */
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
  /**
   * @brief Constructor allowing derived classes to create a Socket object for a specific handle
   * @param handle - unix socket handle
   */
  explicit Socket(int handle) : m_handle(handle) {}

protected:
  int m_handle{InvalidSocketHandle};
};

} // end namespace pw_socket

#endif // SOCKET_H
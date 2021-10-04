#ifndef UNIX_SOCKET_H
#define UNIX_SOCKET_H
#pragma once

#include "socket/Socket.h"

#include <memory>
#include <string>

namespace pw_socket {

/**
 * @brief Class representing a basic unix domain socket, providing auto close on destruction
 * @note Class is not copyable
 * @author Nik Twerdochlib
 * @since Sun Oct 03 2021
 */
class UnixSocket
  : public Socket
{
public:
  /**
   * @brief Constructs a UnixSocket object using the path represented in endpoint
   * @param endpoint - Path for the socket
   */
  explicit UnixSocket(const std::string& endpoint);
  ~UnixSocket() override;

  /**
   * @copydoc Socket::create
   */
  bool create() override;
  /**
   * @copydoc Socket::connect
   */
  bool connect() override;
  /**
   * @brief Setup the socket to listen for connections
   * @param everyone - true to allow any process to connect, false to keep the permissions of the process calling listen()
   * @return true on sucess, false on failure
   */
  bool listen(bool everyone, uint32_t queueSize);
  /**
   * @copydoc Socket::send
   */
  bool send(const void* buffer, std::size_t bytes, std::size_t* bytesSent = nullptr) override;

  /**
   * @brief Convinence function for sending a std::string
   * @param buffer - string object containing data
   * @return true on sucess, false on failure
   */
  bool send(const std::string& buffer) {
    return send(buffer.data(), buffer.size() + 1);
  }
  /**
   * @copydoc Socket::recv
   */
  bool recv(void* buffer, std::size_t bytes, std::size_t* bytesRcvd = nullptr) override;
  /**
   * @copydoc Socket::close
   */
  bool close() override;

  /**
   * @brief Returns the socket name
   * @return socket name
   */
  const char* name() const {
    return m_endpoint.c_str() + (m_endpoint[0] == 0x00 ? 1 : 0);
  }

  /**
   * @brief Accepts a pending connection on the underlying socket
   * @return New UnixSocket representing the new connection
   */
  std::shared_ptr<UnixSocket> acceptConnection();

protected:
  /**
   * @copydoc Socket::Socket(int handle)
   */
  explicit UnixSocket(int handle);

private:
  std::string m_endpoint;
};

} // end namespace pw_socket
#endif // UNIX_SOCKET_H
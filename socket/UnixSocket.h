#ifndef UNIX_SOCKET_H
#define UNIX_SOCKET_H
#pragma once

#include "socket/Socket.h"

#include <memory>
#include <string>

namespace pw_socket {
class UnixSocket
  : public Socket
{
public:
  explicit UnixSocket(const std::string& endpoint);
  ~UnixSocket() override;

  bool create() override;
  bool connect() override;
  bool listen() override;
  bool send(const void* buffer, std::size_t bytes, std::size_t* bytesSent = nullptr) override;
  bool recv(void* buffer, std::size_t bytes, std::size_t* bytesRcvd = nullptr) override;
  bool close() override;

  const char* name() const {
    return m_endpoint.c_str() + (m_endpoint[0] == 0x00 ? 1 : 0);
  }

protected:
  explicit UnixSocket(int handle);

private:
  std::string m_endpoint;
};

} // end namespace pw_socket
#endif // UNIX_SOCKET_H
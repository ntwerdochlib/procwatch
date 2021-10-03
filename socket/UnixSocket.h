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
  bool send(const void* buffer, std::size_t bytes) override;
  bool recv(void* buffer, std::size_t bytes) override;
  bool close() override;

  std::shared_ptr<UnixSocket> waitForConnection();
  static std::shared_ptr<UnixSocket> acceptConnection(int handle);

protected:
  explicit UnixSocket(int handle);

private:
  std::string m_endpoint;
};

} // end namespace pw_socket
#endif // UNIX_SOCKET_H
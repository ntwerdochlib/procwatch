#include "socket/UnixSocket.h"

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <iostream>

namespace pw_socket {
UnixSocket::UnixSocket(const std::string& endpoint)
  : m_endpoint(endpoint)
{}

UnixSocket::UnixSocket(int handle)
  : Socket(handle)
{
  std::cout << __func__ << ": handle: " << m_handle << std::endl;
}

UnixSocket::~UnixSocket()
{
}

bool UnixSocket::create()
{
  m_handle = socket(AF_UNIX, SOCK_STREAM, 0);
  if (InvalidSocketHandle == m_handle) {
    err(errno, "Failed to create unix socket: %s", m_endpoint.c_str());
    return false;
  }
  return true;
}

bool UnixSocket::connect()
{
  if (m_handle == InvalidSocketHandle) {
    errx(ENOTSOCK, "Invalid socket handle");
    return false;
  }

  sockaddr_un sa{
    .sun_family = AF_UNIX,
  };
  memcpy(sa.sun_path, m_endpoint.c_str(), m_endpoint.size());
  auto const r = ::connect(m_handle, reinterpret_cast<sockaddr*>(&sa), sizeof(sa));
  if (r == -1) {
    err(errno, "Failed to connect to socket: %s", m_endpoint.c_str());
    return false;
  }

  return true;
}

bool UnixSocket::listen()
{
  if (m_handle == InvalidSocketHandle) {
    errx(ENOTSOCK, "Invalid socket handle");
    return false;
  }

  sockaddr_un sa{
    .sun_family = AF_UNIX,
  };
  memcpy(sa.sun_path, m_endpoint.c_str(), m_endpoint.size());

  auto r = unlink(m_endpoint.c_str());
  if (r != 0 && errno != ENOENT) {
    err(errno, "%d Failed to remove existing socket endpoint for %s", errno, m_endpoint.c_str());
    return false;
  }

  r = bind(m_handle, reinterpret_cast<sockaddr*>(&sa), sizeof(sa));
  if (r == -1) {
    err(errno, "Unable to bind to socket %s", m_endpoint.c_str());
    return false;
  }

  r = ::listen(m_handle, 10);
  if (r == -1) {
    err(errno, "Unable to configure socket %s for accepting connections", m_endpoint.c_str());
    return false;
  }

  return true;
}

bool UnixSocket::close()
{
  auto const r = Socket::close();
  if (!r) {
    err(errno, "Failed to close socket: %s", m_endpoint.c_str());
  }
  return r;
}

bool UnixSocket::send(const void* buffer, std::size_t bytes, std::size_t* bytesSent)
{
  auto const r = write(m_handle, buffer, bytes);
  if (bytesSent) {
    *bytesSent = r;
  }
  if (r <= 0) {
    err(errno, "Failed sending %d bytes on socket: %s", bytes, m_endpoint.c_str());
    return false;
  }
  return true;
}

bool UnixSocket::recv(void* buffer, std::size_t bytes, std::size_t* bytesRcvd)
{
  auto const r = read(m_handle, buffer, bytes);
  if (*bytesRcvd) {
    *bytesRcvd = r;
  }
  if (r <= 0) {
    err(errno, "Failed recieving %d bytes on socket: %s", bytes, m_endpoint.c_str());
    return false;
  }
  return true;
}

std::shared_ptr<UnixSocket> UnixSocket::waitForConnection()
{
  auto conn = accept(m_handle, nullptr, nullptr);
  if (conn == -1) {
    throw std::runtime_error(strerror(errno));
  }
  return std::shared_ptr<UnixSocket>(new UnixSocket(conn));
}

std::shared_ptr<UnixSocket> UnixSocket::acceptConnection(int handle)
{
  auto conn = accept(handle, nullptr, nullptr);
  if (conn == -1) {
    throw std::runtime_error(strerror(errno));
  }
  return std::shared_ptr<UnixSocket>(new UnixSocket(conn));
}

} // end namespace pw_socket
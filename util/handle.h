#ifndef PW_HANDLE_H
#define PW_HANDLE_H
#pragma once

#include <err.h>
#include <errno.h>
#include <unistd.h>

#if defined (UNIT_TESTING)
#define MOCK_VIRTUAL virtual
#else
#define MOCK_VIRTUAL
#endif

namespace pw_util {
/**
 * @brief Class representing a basic unix handle, providing auto close on destruction
 * @note Class is not copyable
 * @author Nik Twerdochlib
 * @since Sun Oct 03 2021
 */
class Handle
{
public:
  static constexpr int InvalidHandleValue {-1};

  Handle() = default;
  Handle(const Handle&) = delete;
  Handle(Handle&&) = default;
  explicit Handle(int handle)
    : m_handle(handle)
  {}

  ~Handle() {
    close();
  }

  MOCK_VIRTUAL bool close() {
    if (m_handle != InvalidHandleValue) {
      auto const r = ::close(m_handle);
      if (r == -1) {
        err(errno, "Failed closing handle: %d", m_handle);
      }
      m_handle = InvalidHandleValue;
      return r != -1;
    }
    return true;
  }

  Handle& operator=(const Handle&) = delete;
  Handle& operator=(Handle&&) = default;

  bool operator==(int handle) const {
    return m_handle == handle;
  }

  operator int () {
    return value();
  }

  const int& value() const { return m_handle; }

private:
  int m_handle {InvalidHandleValue};
};

} // end namespace pw_util
#endif // PW_HANDLE_H
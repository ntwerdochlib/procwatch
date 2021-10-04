#ifndef PROC_WATCH_SVR_H
#define PROC_WATCH_SVR_H
#pragma once

#include <linux/cn_proc.h>

#include <thread>

#include "socket/UnixSocket.h"

class ProcWatchSvr
{
  using client_t = std::shared_ptr<pw_socket::UnixSocket>;
public:
  ProcWatchSvr() = default;
  ProcWatchSvr(const ProcWatchSvr&) = delete;
  ProcWatchSvr(ProcWatchSvr&&) = delete;

  ~ProcWatchSvr() = default;

  ProcWatchSvr& operator=(const ProcWatchSvr&) = delete;
  ProcWatchSvr& operator=(ProcWatchSvr&&) = delete;

  void run(std::stop_token stopToken);

private:
  void handleNetlinkEvent(const proc_event& ev, const client_t& client);
  inline std::string getProcBinaryPath(uint32_t pid) const;
  inline std::string getProcCommandLine(uint32_t pid) const;
};

#endif // PROC_WATCH_SVR_H
#ifndef PROC_WATCH_SVR_H
#define PROC_WATCH_SVR_H
#pragma once

#include <atomic>
#include <list>
#include <thread>

#include "socket/UnixSocket.h"

class ProcWatchSvr
{
public:
  using client_t = std::shared_ptr<pw_socket::UnixSocket>;

  ProcWatchSvr();
  ProcWatchSvr(const ProcWatchSvr&) = delete;
  ProcWatchSvr(ProcWatchSvr&&) = delete;

  ~ProcWatchSvr();

  ProcWatchSvr& operator=(const ProcWatchSvr&) = delete;
  ProcWatchSvr& operator=(ProcWatchSvr&&) = delete;

  void start();
  void shutdown();

  bool running() const { return m_thread.joinable(); }

  auto const& clients() const { return m_clients; }

protected:
  void run(std::stop_token stopToken);

private:
  std::atomic_bool m_running { false };
  std::jthread m_thread;
  std::list<client_t> m_clients;
};

#endif // PROC_WATCH_SVR_H
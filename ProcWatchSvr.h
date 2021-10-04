#ifndef PROC_WATCH_SVR_H
#define PROC_WATCH_SVR_H
#pragma once

#include <linux/cn_proc.h>

#include <thread>

#include "socket/UnixSocket.h"

/**
 * @brief ProcWatch server.
 * Accepts a connection from a single client and sends information for all new process
 * created after the client has connected
 * @note Class is not copyable
 * @note Class is not movable
 * @author Nik Twerdochlib
 * @since Sun Oct 03 2021
 */
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

  /**
   * @brief Core function of the procwatch server
   * @param stopToken - token used by std::jthread to trigger a request to exit
   * @return (void)
   */
  void run(std::stop_token stopToken);

private:
  /**
   * @brief Processes a proc event message, sending process detail to the client
   * @param ev - proc_events message
   * @param client - Connected client
   * @return (void)
   */
  void handleNetlinkEvent(const proc_event& ev, const client_t& client);
  /**
   * @brief Attempts to retrieve the path for the pid
   * @param pid - PID of target proccess
   * @return 
   */
  inline std::string getProcBinaryPath(uint32_t pid) const;
  /**
   * @brief Attempts to retrieve the command for a process
   * @param pid - PID of target process
   * @return 
   */
  inline std::string getProcCommandLine(uint32_t pid) const;
};

#endif // PROC_WATCH_SVR_H
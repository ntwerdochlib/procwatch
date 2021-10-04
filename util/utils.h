#ifndef PW_UTILS_H
#define PW_UTILS_H
#pragma once

#include <signal.h>
#include <sys/signalfd.h>
#include <sys/epoll.h>
#include <errno.h>
#include <string.h>

#include "handle.h"

/**
 * @brief Utility functions supporting procwatch
 */
namespace pw_util {
  /**
   * @brief Creates a signalfd handle with the provided signals set
   * @param signals - signals the be set in the signalfd
   * @return Handle
   */
  static Handle CreateSignalFd(std::initializer_list<int> signals) {
    sigset_t all;
    sigfillset(&all);
    sigprocmask(SIG_SETMASK, &all, NULL);

    sigset_t sw;
    sigemptyset(&sw);
    for (auto s : signals) {
      sigaddset(&sw, s);
    }
    auto const handle = signalfd(-1, &sw, 0);
    if (handle == -1) {
      throw std::runtime_error(std::string("Failed creating signal handle - ") + strerror(errno));
    }
    return Handle(handle);
  }

  /**
   * @brief Creates an epoll Handle object
   * @param size - Since Linux 2.6.8, the size argument is ignored, but must be greater than zero
   * @return Handle
   */
  static Handle CreateEpoll(int size = 1) {
    int handle = epoll_create(size);
    if (handle == -1) {
      throw std::runtime_error(std::string("Failed creating epoll handle - ") + strerror(errno));
    }
    return Handle(handle);
  }

  /**
   * @brief Adds a handle to a epoll handle object
   * @param epollHandle - A HANDLE object representing an epoll handle
   * @param handle - The handle to be added to the epoll handle
   * @param events - Events to be listened for
   * @return (void)
   * @note Throws std::runtime_error on error
   */
  static void AddHandleToEpoll(Handle& epollHandle, const int& handle, uint32_t events) {
    epoll_event ev {
      .events = events,
      .data = {
        .fd = handle
      }
    };

    // std::cerr << "Adding handle to epoll: " << handle << std::endl;
    auto r = epoll_ctl(epollHandle, EPOLL_CTL_ADD, handle, &ev);
    if (r == -1) {
      throw std::runtime_error(std::string("Failed to add handle to epoll: ").append(std::to_string(handle) + " - " + strerror(errno)));
    }
  }

  /**
   * @brief Modifies a handle to a epoll handle object
   * @param epollHandle - A HANDLE object representing an epoll handle
   * @param handle - The handle to be modified to the epoll handle
   * @param events - Events to be listened for
   * @return (void)
   * @note Throws std::runtime_error on error
   */
  static void ModifyHandleInEpoll(Handle& epollHandle, const int& handle, uint32_t events) {
    epoll_event ev {
      .events = events,
      .data = {
        .fd = handle
      }
    };

    auto r = epoll_ctl(epollHandle, EPOLL_CTL_MOD, handle, &ev);
    if (r == -1) {
      throw std::runtime_error(std::string("Failed to delete handle from epoll: ").append(std::to_string(handle) + " - " + strerror(errno)));
    }
  }

  /**
   * @brief Removes a handle to a epoll handle object
   * @param epollHandle - A HANDLE object representing an epoll handle
   * @param handle - The handle to be removed to the epoll handle
   * @param events - Events to be listened for
   * @return (void)
   * @note Throws std::runtime_error on error
   */
  static void RemoveHandleFromEpoll(Handle& epollHandle, const int& handle, uint32_t events) {
    epoll_event ev {
      .events = events,
      .data = {
        .fd = handle
      }
    };

    auto r = epoll_ctl(epollHandle, EPOLL_CTL_DEL, handle, &ev);
    if (r == -1) {
      throw std::runtime_error(std::string("Failed to delete handle from epoll: ").append(std::to_string(handle) + " - " + strerror(errno)));
    }
  }
} // end namespace pw_util

#endif // PW_UTILS_H
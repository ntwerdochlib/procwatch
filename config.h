#ifndef PW_CONFIG_H
#define PW_CONFIG_H
#pragma once

namespace pw_config {
static constexpr char ServerEndpoint[] { "/tmp/procwatch.socket" };
static constexpr int MaxMessageSize = 1024;
} // end namespace pw_config

#endif //PW_CONFIG_H
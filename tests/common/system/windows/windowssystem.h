/*
 * This is the source code of SpecNet project
 * It is licensed under MIT License.
 *
 * Copyright (c) Dmitriy Bondarenko
 * feel free to contact me: specnet.messenger@gmail.com
 */

#ifndef WINDOWSSYSTEM_H
#define WINDOWSSYSTEM_H
#include <string>
#include <memory>

namespace spec {
std::string  getExePath();
std::string  getExecName();
std::string  execCmd(const char  *cmd);

// Send command to service:
std::string  sendCmd(const char  *serviceName,  const char  *cmd);

// Get unix socket path for service:
std::string  getSockPath(const char  *serviceName);
} // namespace spec

#endif // WINDOWSSYSTEM_H

/*
 * This is the source code of SpecNet project
 *
 * Copyright (c) Dmitriy Bondarenko
 * feel free to contact me: specnet.messenger@gmail.com
 */

#include "linuxsystem.h"
#include <unistd.h>
#include <limits.h>
#include <linux/limits.h>
#include <dlfcn.h>
#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <array>
#include <sys/socket.h>
#include <sys/un.h>
#include <thread>
#include <algorithm>

#define BUFFER_SIZE 256

std::string spec::getExePath()
{
  std::string re;
  char buf[PATH_MAX];
  ssize_t len = 0;
  len = readlink("/proc/self/exe", buf, PATH_MAX);
  if (len>0)  {
      for(--len;  len>0;  --len)  {
          if ('/'  ==  buf[len])  {
              re = std::string(buf,  static_cast<uint32_t>(len));
              break;
          }
      }
  }
  return re;
} // getExePath

std::string  spec::getExecName()
{
  std::string re;
  char buf[PATH_MAX];
  ssize_t len = 0;
  len = readlink("/proc/self/exe", buf, PATH_MAX);
  if (len > 0)
  {
      re = std::string(buf, len);
  }
  return re;
} // getExecName


std::string  spec::execCmd(const char  *cmd)
{
  std::array<char, 128> buffer;
  std::string  re;
  std::shared_ptr<FILE> pipe(popen(cmd, "r"), pclose);
  if (!pipe) throw std::runtime_error("popen() failed!");
  while (!feof(pipe.get()))
  {
      if (fgets(buffer.data(), 128, pipe.get()) != nullptr)
      {
            re += buffer.data();
      }
  }
  //in deleter of shared_ptr: pclose(pipe);
  return re;
} // execCmd

std::string  spec::sendCmd(const char  *serviceName,  const char  *cmd)
{
  std::string re;
  int sock = -1;
  //faux loop
  do {
      //int sock = ::socket(PF_UNIX, SOCK_STREAM, 0);
      int sock = ::socket(AF_UNIX, SOCK_SEQPACKET, 0);
      if (-1 == sock) {
          //std::cerr << "Error: LinuxService::sendCmd  -1 == ::socket(" << std::endl;
          break;
      }
      struct sockaddr_un addr;
      ::memset(&addr, 0, sizeof(struct sockaddr_un));
      addr.sun_family = AF_UNIX;
      const std::string & sock_path = spec::getSockPath(serviceName);
      ::memcpy(addr.sun_path, sock_path.c_str(), sock_path.length());

#if defined (Android)
      int res = ::connect(sock, (struct sockaddr *)&addr, sizeof(addr));
#else
      int res = ::connect(sock, (struct sockaddr *)&addr, SUN_LEN(&addr));
#endif

      if (-1 == res ) {
          //std::cerr << "Error: LinuxService::sendCmd  ::connect(sock to "
          //          << sock_path << std::endl;
          break;
      }

      /* Send. */
      res = write(sock, cmd, strlen(cmd) + 1);
      if (-1 == res ) {
          //std::cerr << "Error: LinuxService::sendCmd  ::write(sock to "
          //          << sock_path << std::endl;
          break;
      }

      char buffer[BUFFER_SIZE];
      /* Receive result. */
      res = read(sock, buffer, BUFFER_SIZE);
      if ( res < 0 ) {
          //std::cerr << "Error: LinuxService::sendCmd  ::read(sock from "
           //         << sock_path << std::endl;
          break;
      }

      //buffer[res] = 0;
      re = std::string(buffer, res);
      //printf("Server answer: %s\n", buffer);

      //re = true;
  } while (false);
  if (-1 != sock) { ::close(sock); }

  return re;
} // sendCmd


std::string  spec::getSockPath(const char  *serviceName)
{
  std::string legal (serviceName);
  std::transform(legal.begin(), legal.end(), legal.begin(),
    [](char ch)  {
      const char * legal = "abcdefghijklmnopqrstuvwxyz1234567890";
      for (const char *p = legal; *p; ++p) {
          if (*p==ch) { return ch; }
      }
      return 'a'; } );
  std::string re ("/var/tmp/");
  re.append(legal);
  return re;
} // getSockPath


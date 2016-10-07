#pragma once

#include "std/unique_ptr.hpp"

namespace location
{
class GpsInfo;
}

namespace platform
{
class Socket;
}

namespace tracking
{
class Reporter final
{
public:
  static size_t constexpr kPushDelayMs = 10000;
  static const char kAllowKey[];

  Reporter(unique_ptr<platform::Socket> socket, size_t pushDelayMs);
  ~Reporter();
  void AddLocation(location::GpsInfo const & info);

  class Worker
  {
  public:
    virtual void AddLocation(location::GpsInfo const & info) = 0;
    virtual void Stop() = 0;
  };

private:
  Worker * m_worker;
};

}  // namespace tracking

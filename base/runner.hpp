#pragma once

#include "../std/function.hpp"

namespace threads
{

typedef function<void()> RunnerFuncT;

/// Automatically uses system thread pools if it's available
class IRunner
{
public:
  virtual ~IRunner() {}

  virtual void Run(RunnerFuncT f) = 0;
  /// Waits until all running threads will stop
  virtual void Join() = 0;
};

/// Synchronous implementation
class SimpleRunner : public IRunner
{
public:
  virtual void Run(RunnerFuncT f)
  {
    f();
  }

  virtual void Join()
  {
  }
};

}

#pragma once

#include "std/function.hpp"

namespace threads
{

typedef function<void()> RunnerFuncT;

// Base Runner interface: performes given tasks.
class IRunner
{
public:
  virtual ~IRunner() {}

  virtual void Run(RunnerFuncT const & f) const = 0;

  // Helper function that calls f() and catches all exception.
  static void CallAndCatchAll(RunnerFuncT const & f);

protected:

  // Waits until all running threads stop.
  // Not for public use! Used in unit tests only, since
  // some runners use global thread pool and interfere with each other.
  virtual void Join() = 0;
};

// Synchronous implementation: just immediately executes given tasks.
class SimpleRunner : public IRunner
{
public:
  virtual void Run(RunnerFuncT const & f) const
  {
    IRunner::CallAndCatchAll(f);
  }

protected:
  virtual void Join()
  {
  }
};

}

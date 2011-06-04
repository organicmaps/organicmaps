#pragma once

#include "../base/runner.hpp"

namespace threads
{

/// @note All current implementations use one shared system pool
class ConcurrentRunner : public IRunner
{
  class Impl;
  Impl * m_pImpl;

public:
  ConcurrentRunner();
  virtual ~ConcurrentRunner();
  virtual void Run(RunnerFuncT const & f) const;

protected:
  virtual void Join();
};

}

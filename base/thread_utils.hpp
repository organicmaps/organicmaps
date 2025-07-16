#pragma once

#include <thread>
#include <vector>

#include "base/macros.hpp"

namespace threads
{
template <typename Thread = std::thread, typename ThreadContainer = std::vector<Thread>>
class ThreadsJoiner
{
public:
  explicit ThreadsJoiner(ThreadContainer & threads) : m_threads(threads) {}

  void Join()
  {
    for (auto & thread : m_threads)
      if (thread.joinable())
        thread.join();
  }

  ~ThreadsJoiner() { Join(); }

private:
  ThreadContainer & m_threads;
};

// This class can be used in thread pools to store std::packaged_task<> objects.
// std::packaged_task<> isn’t copyable so we have to use std::move().
// This idea is borrowed from the book C++ Concurrency in action by Anthony Williams (Chapter 9).
class FunctionWrapper
{
public:
  template <typename F>
  FunctionWrapper(F && func) : m_impl(new ImplType<F>(std::move(func)))
  {}

  FunctionWrapper() = default;

  FunctionWrapper(FunctionWrapper && other) : m_impl(std::move(other.m_impl)) {}

  FunctionWrapper & operator=(FunctionWrapper && other)
  {
    m_impl = std::move(other.m_impl);
    return *this;
  }

  void operator()() { m_impl->Call(); }

private:
  struct ImplBase
  {
    virtual ~ImplBase() = default;

    virtual void Call() = 0;
  };

  template <typename F>
  struct ImplType : ImplBase
  {
    ImplType(F && func) : m_func(std::move(func)) {}

    void Call() override { m_func(); }

    F m_func;
  };

  std::unique_ptr<ImplBase> m_impl;

  DISALLOW_COPY(FunctionWrapper);
};
}  // namespace threads

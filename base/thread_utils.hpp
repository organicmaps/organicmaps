#pragma once

#include <thread>
#include <vector>

#include <boost/noncopyable.hpp>

namespace threads
{
template<typename Thread = std::thread, typename ThreadColl= std::vector<Thread>>
class ThreadsJoiner
{
public:
  explicit ThreadsJoiner(ThreadColl & threads) : m_threads(threads) {}
  ~ThreadsJoiner()
  {
    for (auto & thread : m_threads)
    {
      if (thread.joinable())
        thread.join();
    }
  }

private:
  ThreadColl & m_threads;
};

using StandartThreadsJoiner = ThreadsJoiner<>;

class FunctionWrapper : boost::noncopyable
{
public:
  template<typename F>
  FunctionWrapper(F && func) : m_impl(new ImplType<F>(std::move(func))) {}
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

  template<typename F>
  struct ImplType : ImplBase
  {
    ImplType(F && func) : m_func(std::move(func)) {}
    void Call() override { m_func(); }

    F m_func;
  };

  std::unique_ptr<ImplBase> m_impl;
};
}  // namespace threads

#pragma once

#include "base/macros.hpp"

#include "drape/drape_global.hpp"
#include "drape/pointers.hpp"

#include <mutex>

namespace df
{
class DrapeEngine;

class DrapeEngineSafePtr
{
public:
  void Set(ref_ptr<DrapeEngine> engine)
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_engine = engine;
  }

  explicit operator bool()
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_engine != nullptr;
  }

  template <typename Function, typename... Args>
  void SafeCall(Function && f, Args &&... functionArgs)
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_engine != nullptr)
      (m_engine.get()->*f)(std::forward<Args>(functionArgs)...);
  }

  template <typename Function, typename... Args>
  dp::DrapeID SafeCallWithResult(Function && f, Args &&... functionArgs)
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_engine != nullptr)
      return (m_engine.get()->*f)(std::forward<Args>(functionArgs)...);
    return dp::DrapeID();
  }

private:
  ref_ptr<DrapeEngine> m_engine;
  std::mutex m_mutex;

  friend class DrapeEngineLockGuard;
};

class DrapeEngineLockGuard
{
public:
  explicit DrapeEngineLockGuard(DrapeEngineSafePtr & enginePtr) : m_ptr(enginePtr) { m_ptr.m_mutex.lock(); }

  ~DrapeEngineLockGuard() { m_ptr.m_mutex.unlock(); }

  explicit operator bool() { return m_ptr.m_engine != nullptr; }

  ref_ptr<DrapeEngine> Get() { return m_ptr.m_engine; }

private:
  DrapeEngineSafePtr & m_ptr;
  DISALLOW_COPY_AND_MOVE(DrapeEngineLockGuard);
};
}  // namespace df

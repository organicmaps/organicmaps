#pragma once

#include "thread.hpp"
#include "../std/vector.hpp"

namespace threads
{
  /// Manages any number of threads
  /// current implementation is simple and naive
  class Pool
  {
    vector<threads::Thread> m_threadHandles;

  public:
    /// Takes ownership on pRoutine
    bool StartThread(threads::IRoutine * pRoutine)
    {
      threads::Thread h;
      if (h.Create(pRoutine))
      {
        m_threadHandles.push_back(h);
        return true;
      }
      return false;
    }

    void Join()
    {
      for (size_t i = 0; i < m_threadHandles.size(); ++i)
      {
        m_threadHandles[i].Join();
      }
    }
  };
} // namespace threads

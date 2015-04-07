#pragma once

#include "base/base.hpp"
#include "base/mutex.hpp"

#include "std/map.hpp"


#define TRACKER_MULTITHREADED

namespace dbg
{
  class ObjectTracker
  {
    /// Pointer -> Info (Serial number of creation)
    static map<void *, size_t> m_map;
    static size_t m_counter;

#ifdef TRACKER_MULTITHREADED
    static threads::Mutex m_mutex;
#endif

    static void Add(void *);
    static void Remove(void *);

  public:
    ObjectTracker();
    ObjectTracker(ObjectTracker const & rhs);
    ~ObjectTracker();

    static void PrintLeaks();
  };

  void BreakIntoDebugger();
}

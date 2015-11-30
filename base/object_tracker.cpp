#include "base/object_tracker.hpp"
#include "base/assert.hpp"
#include "base/logging.hpp"

#include "std/target_os.hpp"

#ifndef OMIM_OS_WINDOWS
  #include <signal.h>
  #include <unistd.h>
#else
  #include "std/windows.hpp"
#endif


namespace dbg
{

map<void *, size_t> ObjectTracker::m_map;
size_t ObjectTracker::m_counter = 0;

#ifdef TRACKER_MULTITHREADED
threads::Mutex ObjectTracker::m_mutex;
#endif

void ObjectTracker::Add(void * p)
{
#ifdef TRACKER_MULTITHREADED
  threads::MutexGuard guard(m_mutex);
#endif

  CHECK ( m_map.insert(make_pair(p, m_counter++)).second == true, (p) );
}

void ObjectTracker::Remove(void * p)
{
#ifdef TRACKER_MULTITHREADED
  threads::MutexGuard guard(m_mutex);
#endif

  CHECK ( m_map.erase(p) == 1, (p) );
}

ObjectTracker::ObjectTracker()
{
  Add(this);
}

ObjectTracker::ObjectTracker(ObjectTracker const &)
{
  Add(this);
}

ObjectTracker::~ObjectTracker()
{
  Remove(this);
}

void ObjectTracker::PrintLeaks()
{
#ifdef TRACKER_MULTITHREADED
  threads::MutexGuard guard(m_mutex);
#endif

  if (m_map.empty())
    LOG(LINFO, ("No leaks found!"));
  else
    LOG(LINFO, ("Leaks map:", m_map));
}

void BreakIntoDebugger()
{
  /// @todo Probably we can make it more simple (like std::terminate).

#if defined(OMIM_OS_WINDOWS)
  DebugBreak();
#else
  kill(getpid(), SIGINT);
#endif
}

}

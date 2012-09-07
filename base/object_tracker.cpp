#include "object_tracker.hpp"
#include "assert.hpp"
#include "logging.hpp"


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

  CHECK ( m_map.insert(make_pair(p, m_counter++)).second == true, () );
}

void ObjectTracker::Remove(void * p)
{
#ifdef TRACKER_MULTITHREADED
  threads::MutexGuard guard(m_mutex);
#endif

  CHECK ( m_map.erase(p) == 1, () );
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

}

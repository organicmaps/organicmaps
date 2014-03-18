#include "fence_manager.hpp"

#include "../base/assert.hpp"
#include "../base/logging.hpp"

FenceManager::FenceManager(int conditionPoolSize)
  : m_currentFence(0),
    m_isCancelled(false)
{
  for (unsigned i = 0; i < conditionPoolSize; ++i)
    m_conditionPool.push_back(new threads::Condition());
}

FenceManager::~FenceManager()
{
  for (map<int, threads::Condition*>::const_iterator it = m_activeFences.begin();
       it != m_activeFences.end();
       ++it)
  {
    it->second->Signal();
    delete it->second;
  }

  for (list<threads::Condition*>::iterator it = m_conditionPool.begin(); it != m_conditionPool.end(); ++it)
    delete *it;
}

int FenceManager::insertFence()
{
  threads::MutexGuard g(m_mutex);

  if (m_isCancelled)
    return -1;

  if (m_conditionPool.empty())
    return -1;

  threads::Condition * cond = m_conditionPool.front();
  m_conditionPool.pop_front();

  int id = m_currentFence++;

  m_activeFences[id] = cond;

  return id;
}

void FenceManager::signalFence(int id)
{
  threads::MutexGuard mutexGuard(m_mutex);

  if (m_isCancelled)
    return;

  map<int, threads::Condition*>::iterator it = m_activeFences.find(id);

  if (it == m_activeFences.end())
  {
    LOG(LDEBUG, ("fence with id", id, "has been already signalled or hasn't been installed yet"));
    return;
  }

  threads::Condition * cond = it->second;

  /// i suppose that this guard will be destroyed after mutexGuard
  threads::ConditionGuard fenceGuard(*cond);

  /// erasing fence from active fences
  m_activeFences.erase(it);

  /// returning condition to the pool
  m_conditionPool.push_back(cond);

  /// signalling to all waiting fences
  fenceGuard.Signal(true);
}

void FenceManager::joinFence(int id)
{
  threads::Condition * cond = 0;
  {
    threads::MutexGuard g(m_mutex);

    if (m_isCancelled)
      return;

    map<int, threads::Condition*>::iterator it = m_activeFences.find(id);

    if (it == m_activeFences.end())
    {
      LOG(LDEBUG, ("fence with id", id, "has been already reached in the past or hasn't been installed yet"));
      return;
    }

    cond = it->second;

    /// we should lock condition here, to prevent us from the situation
    /// when the condition will be signaled before it's been waited for
    cond->Lock();
  }

  /// to prevent from "spurious wakeups"
  while (m_activeFences.find(id) != m_activeFences.end())
    cond->Wait();

  cond->Unlock();
}

void FenceManager::cancel()
{
  threads::MutexGuard g(m_mutex);

  if (m_isCancelled)
    return;

  m_isCancelled = true;

  list<pair<int, threads::Condition*> > tempList;

  for (map<int, threads::Condition*>::iterator it = m_activeFences.begin();
       it != m_activeFences.end();
       ++it)
       tempList.push_back(make_pair(it->first, it->second));

  for (list<pair<int, threads::Condition*> >::const_iterator it = tempList.begin();
       it != tempList.end();
       ++it)
  {
    threads::ConditionGuard fenceGuard(*it->second);
    m_activeFences.erase(it->first);
    fenceGuard.Signal(true);
  }
}

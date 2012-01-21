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
  threads::MutexGuard g(m_mutex);

  if (m_isCancelled)
    return;

  map<int, threads::Condition*>::iterator it = m_activeFences.find(id);

  if (it == m_activeFences.end())
  {
    LOG(LINFO, ("fence with id", id, "has been already signalled or hasn't been installed yet"));
    return;
  }

  threads::Condition * cond = it->second;

  /// erasing fence from active fences
  m_activeFences.erase(it);

  /// returning condition to the pool
  m_conditionPool.push_back(cond);

  /// signalling to all waiting fences
  cond->Signal(true);
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
      LOG(LINFO, ("fence with id", id, "has been already reached in the past or hasn't been installed yet"));
      return;
    }

    cond = it->second;
  }

  threads::ConditionGuard g(*cond);
  g.Wait();
}

void FenceManager::cancel()
{
  threads::MutexGuard g(m_mutex);

  m_isCancelled = true;

  map<int, threads::Condition*>::iterator it = m_activeFences.begin();

  for (; it != m_activeFences.end(); ++it)
    it->second->Signal(true);
}

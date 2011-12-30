#include "fence_manager.hpp"

#include "../base/assert.hpp"
#include "../base/logging.hpp"

namespace yg
{
  FenceManager::FenceManager(int conditionPoolSize)
    : m_currentFence(0)
  {
    m_conditionPool.resize(conditionPoolSize);
    for (unsigned i = 0; i < m_conditionPool.size(); ++i)
      m_conditionPool[i] = new threads::Condition();
  }

  FenceManager::~FenceManager()
  {
    for (map<int, threads::Condition*>::const_iterator it = m_activeFences.begin();
         it != m_activeFences.end();
         ++it)
    {
      it->second->Signal(true);
      delete it->second;
    }

    for (unsigned i = 0; i < m_conditionPool.size(); ++i)
    {
      delete m_conditionPool[i];
    }
  }

  int FenceManager::insertFence()
  {
    threads::MutexGuard g(m_mutex);

    if (m_conditionPool.empty())
      return -1;

    threads::Condition * cond = m_conditionPool.back();
    m_conditionPool.pop_back();

    int id = m_currentFence++;

    m_activeFences[id] = cond;

    return id;
  }

  void FenceManager::signalFence(int id)
  {
    threads::MutexGuard g(m_mutex);

    map<int, threads::Condition*>::iterator it = m_activeFences.find(id);

    CHECK(it != m_activeFences.end(), ("using singalFence twice for the same id is error"));

    threads::Condition * cond = it->second;

    /// signalling to all waiting fences
    cond->Signal(true);

    /// erasing fence from active fences
    m_activeFences.erase(it);

    /// returning condition to the pool
    m_conditionPool.push_back(cond);
  }

  void FenceManager::joinFence(int id)
  {
    threads::Condition * cond = 0;
    {
      threads::MutexGuard g(m_mutex);

      map<int, threads::Condition*>::iterator it = m_activeFences.find(id);

      if (it == m_activeFences.end())
      {
        LOG(LINFO, ("fence has been reached in the past or haven't been installed yet"));
        return;
      }

      cond = it->second;
    }

    threads::ConditionGuard g(*cond);
    g.Wait();
  }
}

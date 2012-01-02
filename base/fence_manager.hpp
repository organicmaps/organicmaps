#pragma once

#include "../base/condition.hpp"
#include "../base/mutex.hpp"

#include "../std/map.hpp"
#include "../std/vector.hpp"

class FenceManager
{
private:

  threads::Mutex m_mutex;
  vector<threads::Condition *> m_conditionPool;
  map<int, threads::Condition *> m_activeFences;
  int m_currentFence;

public:

  FenceManager(int conditionPoolSize);
  ~FenceManager();

  int  insertFence();
  void joinFence(int id);
  void signalFence(int id);
};

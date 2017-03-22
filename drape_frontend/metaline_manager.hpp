#pragma once

#include "drape_frontend/read_metaline_task.hpp"

#include "drape/object_pool.hpp"
#include "drape/pointers.hpp"

#include "indexer/feature_decl.hpp"

#include <mutex>
#include <set>

namespace df
{
class ThreadsCommutator;

class MetalineManager
{
public:
  MetalineManager(ref_ptr<ThreadsCommutator> commutator, MapDataProvider & model);
  ~MetalineManager();

  void Update(std::set<MwmSet::MwmId> const & mwms);

  m2::SharedSpline GetMetaline(FeatureID const & fid) const;

private:
  void OnTaskFinished(threads::IRoutine * task);

  using TasksPool = ObjectPool<ReadMetalineTask, ReadMetalineTaskFactory>;

  MapDataProvider & m_model;

  MetalineCache m_metalineCache;
  mutable std::mutex m_metalineCacheMutex;

  std::set<MwmSet::MwmId> m_mwms;
  std::mutex m_mwmsMutex;

  TasksPool m_tasksPool;
  drape_ptr<threads::ThreadPool> m_threadsPool;
  ref_ptr<ThreadsCommutator> m_commutator;
};
}  // namespace df

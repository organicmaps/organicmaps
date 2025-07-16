#pragma once

#include "drape_frontend/read_metaline_task.hpp"

#include "drape/drape_routine.hpp"
#include "drape/pointers.hpp"

#include "indexer/feature_decl.hpp"

#include <memory>
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

  void Stop();

  void Update(std::set<MwmSet::MwmId> const & mwms);

  m2::SharedSpline GetMetaline(FeatureID const & fid) const;

private:
  void OnTaskFinished(std::shared_ptr<ReadMetalineTask> const & task);

  MapDataProvider & m_model;
  ref_ptr<ThreadsCommutator> m_commutator;

  dp::ActiveTasks<ReadMetalineTask> m_activeTasks;

  MetalineCache m_metalineCache;
  mutable std::mutex m_metalineCacheMutex;

  std::set<MwmSet::MwmId> m_mwms;
  std::mutex m_mwmsMutex;
};
}  // namespace df

#include "drape_frontend/metaline_manager.hpp"

#include "drape_frontend/map_data_provider.hpp"
#include "drape_frontend/message_subclasses.hpp"
#include "drape_frontend/threads_commutator.hpp"

#include "base/logging.hpp"

#include <functional>

namespace df
{
MetalineManager::MetalineManager(ref_ptr<ThreadsCommutator> commutator, MapDataProvider & model)
  : m_model(model)
  , m_tasksPool(4, ReadMetalineTaskFactory(m_model))
  , m_threadsPool(make_unique_dp<threads::ThreadPool>(2, std::bind(&MetalineManager::OnTaskFinished,
                                                                   this, std::placeholders::_1)))
  , m_commutator(commutator)
{}

MetalineManager::~MetalineManager()
{
  m_threadsPool->Stop();
  m_threadsPool.reset();
}

void MetalineManager::Update(std::set<MwmSet::MwmId> const & mwms)
{
  std::lock_guard<std::mutex> lock(m_mwmsMutex);
  for (auto const & mwm : mwms)
  {
    auto const result = m_mwms.insert(mwm);
    if (result.second)
    {
      ReadMetalineTask * task = m_tasksPool.Get();
      task->Init(mwm);
      m_threadsPool->PushBack(task);
    }
  }
}

m2::SharedSpline MetalineManager::GetMetaline(FeatureID const & fid) const
{
  std::lock_guard<std::mutex> lock(m_metalineCacheMutex);
  auto const metalineIt = m_metalineCache.find(fid);
  if (metalineIt == m_metalineCache.end())
    return m2::SharedSpline();
  return metalineIt->second;
}

void MetalineManager::OnTaskFinished(threads::IRoutine * task)
{
  ASSERT(dynamic_cast<ReadMetalineTask *>(task) != nullptr, ());
  ReadMetalineTask * t = static_cast<ReadMetalineTask *>(task);

  // Update metaline cache.
  if (!task->IsCancelled())
  {
    std::lock_guard<std::mutex> lock(m_metalineCacheMutex);
    auto const & metalines = t->GetMetalines();
    for (auto const & metaline : metalines)
      m_metalineCache[metaline.first] = metaline.second;

    if (!metalines.empty())
    {
      LOG(LDEBUG, ("Metalines prepared:", t->GetMwmId()));
      m_commutator->PostMessage(ThreadsCommutator::RenderThread,
                                make_unique_dp<UpdateMetalinesMessage>(),
                                MessagePriority::Normal);
    }
  }

  t->Reset();
  m_tasksPool.Return(t);
}
}  // namespace df

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
  , m_commutator(commutator)
{}

MetalineManager::~MetalineManager()
{
  Stop();
}

void MetalineManager::Stop()
{
  m_activeTasks.FinishAll();
}

void MetalineManager::Update(std::set<MwmSet::MwmId> const & mwms)
{
  std::lock_guard<std::mutex> lock(m_mwmsMutex);
  for (auto const & mwm : mwms)
  {
    ASSERT(mwm.IsAlive(), ());
    if (mwm.GetInfo()->GetType() != MwmInfo::MwmTypeT::COUNTRY || !m_mwms.insert(mwm).second)
      continue;

    auto readingTask = std::make_shared<ReadMetalineTask>(m_model, mwm);
    auto routine = dp::DrapeRoutine::Run([this, readingTask]()
    {
      readingTask->Run();
      OnTaskFinished(readingTask);
    });

    if (routine)
      m_activeTasks.Add(std::move(readingTask), std::move(routine));
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

void MetalineManager::OnTaskFinished(std::shared_ptr<ReadMetalineTask> const & task)
{
  if (task->IsCancelled())
    return;

  std::lock_guard<std::mutex> lock(m_metalineCacheMutex);

  if (task->UpdateCache(m_metalineCache))
  {
    // Notify frontend renderer.
    LOG(LDEBUG, ("Metalines prepared:", task->GetMwmId()));
    m_commutator->PostMessage(ThreadsCommutator::RenderThread, make_unique_dp<UpdateMetalinesMessage>(),
                              MessagePriority::Normal);
  }

  // Remove task from active ones.
  m_activeTasks.Remove(task);
}
}  // namespace df

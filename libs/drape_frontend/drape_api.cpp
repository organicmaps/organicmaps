#include "drape_frontend/drape_api.hpp"
#include "drape_frontend/drape_engine.hpp"
#include "drape_frontend/message_subclasses.hpp"

namespace df
{
void DrapeApi::SetDrapeEngine(ref_ptr<DrapeEngine> engine)
{
  m_engine.Set(engine);
}

void DrapeApi::AddLine(std::string const & id, DrapeApiLineData const & data)
{
  DrapeEngineLockGuard lock(m_engine);
  if (!lock)
    return;

  auto & threadCommutator = lock.Get()->m_threadCommutator;
  auto const it = m_lines.find(id);
  if (it != m_lines.end())
  {
    threadCommutator->PostMessage(ThreadsCommutator::ResourceUploadThread, make_unique_dp<DrapeApiRemoveMessage>(id),
                                  MessagePriority::Normal);
  }

  m_lines[id] = data;

  TLines lines;
  lines.insert(std::make_pair(id, data));
  threadCommutator->PostMessage(ThreadsCommutator::ResourceUploadThread, make_unique_dp<DrapeApiAddLinesMessage>(lines),
                                MessagePriority::Normal);
}

void DrapeApi::RemoveLine(std::string const & id)
{
  DrapeEngineLockGuard lock(m_engine);
  if (!lock)
    return;

  auto & threadCommutator = lock.Get()->m_threadCommutator;
  m_lines.erase(id);
  threadCommutator->PostMessage(ThreadsCommutator::ResourceUploadThread, make_unique_dp<DrapeApiRemoveMessage>(id),
                                MessagePriority::Normal);
}

void DrapeApi::Clear()
{
  DrapeEngineLockGuard lock(m_engine);
  if (!lock)
    return;

  auto & threadCommutator = lock.Get()->m_threadCommutator;
  m_lines.clear();
  threadCommutator->PostMessage(ThreadsCommutator::ResourceUploadThread,
                                make_unique_dp<DrapeApiRemoveMessage>("", true /* remove all */),
                                MessagePriority::Normal);
}

void DrapeApi::Invalidate()
{
  DrapeEngineLockGuard lock(m_engine);
  if (!lock)
    return;

  auto & threadCommutator = lock.Get()->m_threadCommutator;
  threadCommutator->PostMessage(ThreadsCommutator::ResourceUploadThread,
                                make_unique_dp<DrapeApiRemoveMessage>("", true /* remove all */),
                                MessagePriority::Normal);

  threadCommutator->PostMessage(ThreadsCommutator::ResourceUploadThread,
                                make_unique_dp<DrapeApiAddLinesMessage>(m_lines), MessagePriority::Normal);
}
}  // namespace df

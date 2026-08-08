#include "indexer/terrain/twm_set.hpp"

#include "indexer/terrain/terrain_serdes.hpp"

#include "geometry/mercator.hpp"

#include "base/file_name_utils.hpp"
#include "base/logging.hpp"

#include <algorithm>

namespace terrain
{
std::string DebugPrint(TwmId const & id)
{
  if (id.m_info)
    return "TwmId [" + base::FileNameFromFullPath(id.m_info->GetFilePath()) + "]";
  return "TwmId [invalid]";
}

std::pair<TwmId, TwmSet::RegResult> TwmSet::Register(std::string const & filePath, int64_t version /* = 0 */)
{
  std::pair<TwmId, RegResult> result;
  WithEventLog([&](EventList & events)
  {
    if (m_condemned.count(filePath) > 0)
    {
      result = {TwmId(), RegResult::Condemned};
      return;
    }

    if (TwmId const id = GetIdByKeyImpl(filePath); id.IsAlive())
    {
      // Resurrect the marked file (cf. the MwmSet same-version re-registration).
      SetStatus(*id.GetInfo(), TwmInfo::STATUS_REGISTERED, events);
      result = {id, RegResult::AlreadyRegistered};
      return;
    }

    m2::RectD limitRect;
    if (!ReadLimitRect(filePath, limitRect))
    {
      LOG(LWARNING, ("Condemning the unreadable terrain file", filePath));
      m_condemned.insert(filePath);
      result = {TwmId(), RegResult::BadFile};
      return;
    }

    // The tracer merges the triangles of all the blocks of a query: an overlap doubles
    // the shared triangles and fails the trace (the duplicate directed edge check).
    for (auto const & [path, infos] : m_registry)
    {
      if (infos.empty())
        continue;
      auto const & info = infos.back();
      if (TwmId(info).IsAlive() && IsInteriorOverlap(limitRect, info->GetLimitRect()))
      {
        LOG(LWARNING, ("The terrain file", filePath, "overlaps the registered", path));
        result = {TwmId(), RegResult::Overlapping};
        return;
      }
    }

    auto const info = std::make_shared<TwmInfo>(filePath, limitRect, version);
    SetStatus(*info, TwmInfo::STATUS_REGISTERED, events);
    AddToRegistryImpl(info);
    result = {TwmId(info), RegResult::Success};
  });
  return result;
}

bool TwmSet::ReadLimitRect(std::string const & filePath, m2::RectD & limitRect)
{
  try
  {
    FilesContainerR const container(filePath);
    TwmHeader header;
    ReaderSource<ModelReaderPtr> src(container.GetReader(kHeaderTag));
    header.Deserialize(src);
    limitRect = header.GetLimitRect();
  }
  catch (RootException const & ex)
  {
    LOG(LWARNING, ("Can't read the terrain header", filePath, ":", ex.Msg()));
    return false;
  }
  return true;
}

bool TwmSet::Deregister(std::string const & filePath)
{
  bool deregistered = false;
  WithEventLog([&](EventList & events)
  {
    TwmId const id = GetIdByKeyImpl(filePath);
    if (id.IsNull())
      return;
    deregistered = DeregisterImpl(id, events);
  });
  return deregistered;
}

void TwmSet::Condemn(std::vector<TwmId> const & ids)
{
  WithEventLog([&](EventList & events)
  {
    for (auto const & id : ids)
    {
      if (id.IsNull())
        continue;
      m_condemned.insert(id.GetInfo()->GetFilePath());
      DeregisterImpl(id, events);
    }
  });
}

template <typename Fn>
void TwmSet::ForEachBlockByRectImpl(m2::RectD const & rect, Fn && fn) const
{
  std::lock_guard<std::mutex> lock(m_lock);
  // The block rects are canonical, so a query rect poking beyond the +-180 seam (see
  // TileKey::GetWrappedDataRect) intersects exactly the blocks of its canonical part -
  // no explicit clipping or splitting is needed here.
  ASSERT(rect.IsValid(), (rect));
  for (auto const & [path, infos] : m_registry)
    if (!infos.empty() && infos.back()->IsRegistered() && rect.IsIntersect(infos.back()->GetLimitRect()))
      fn(infos.back());
}

void TwmSet::GetBlocksByRect(m2::RectD const & rect, std::vector<TwmId> & ids) const
{
  ids.clear();
  ForEachBlockByRectImpl(rect, [&](std::shared_ptr<TwmInfo> const & info) { ids.emplace_back(info); });
}

bool TwmSet::HasBlocks(m2::RectD const & rect) const
{
  bool found = false;
  ForEachBlockByRectImpl(rect, [&](std::shared_ptr<TwmInfo> const &) { found = true; });
  return found;
}

bool TwmSet::HasOlderBlocks(m2::RectD const & rect, int64_t version) const
{
  bool found = false;
  ForEachBlockByRectImpl(rect, [&](std::shared_ptr<TwmInfo> const & info)
  {
    if (info->GetVersion() < version)
      found = true;
  });
  return found;
}

void TwmSet::GetBlockRectsByRect(m2::RectD const & rect, std::vector<m2::RectD> & rects) const
{
  rects.clear();
  ForEachBlockByRectImpl(rect, [&](std::shared_ptr<TwmInfo> const & info) { rects.push_back(info->GetLimitRect()); });
}

TwmSet::Handle TwmSet::GetHandleById(TwmId const & id)
{
  Handle handle;
  WithEventLog([&](EventList & events) { handle = GetHandleByIdImpl(id, events); });
  return handle;
}

std::unique_ptr<TwmValue> TwmSet::CreateValue(TwmInfo & info) const
{
  try
  {
    return std::make_unique<TwmValue>(info.GetFilePath());
  }
  catch (::Reader::TooManyFilesException const &)
  {
    throw;  // Transient, the base keeps the file registered.
  }
  catch (RootException const &)
  {
    // Corrupt data: the base deregisters the file, never register it again.
    m_condemned.insert(info.GetFilePath());
    throw;
  }
}

void TwmSet::SetStatus(TwmInfo & info, TwmInfo::Status status, EventList & events)
{
  TwmInfo::Status const oldStatus = info.SetStatus(status);
  if (oldStatus == status)
    return;

  switch (status)
  {
  case TwmInfo::STATUS_REGISTERED: events.Add(Event(Event::TYPE_REGISTERED, info.GetFilePath())); break;
  case TwmInfo::STATUS_MARKED_TO_DEREGISTER: break;
  case TwmInfo::STATUS_DEREGISTERED: events.Add(Event(Event::TYPE_DEREGISTERED, info.GetFilePath())); break;
  }
}

void TwmSet::ProcessEvents(EventList & events)
{
  for (auto const & event : events.Get())
  {
    switch (event.m_type)
    {
    case Event::TYPE_REGISTERED: m_observers.ForEach(&Observer::OnTerrainRegistered, event.m_filePath); break;
    case Event::TYPE_DEREGISTERED: m_observers.ForEach(&Observer::OnTerrainDeregistered, event.m_filePath); break;
    }
  }
}

std::string DebugPrint(TwmSet::RegResult result)
{
  switch (result)
  {
  case TwmSet::RegResult::Success: return "Success";
  case TwmSet::RegResult::AlreadyRegistered: return "AlreadyRegistered";
  case TwmSet::RegResult::Overlapping: return "Overlapping";
  case TwmSet::RegResult::Condemned: return "Condemned";
  case TwmSet::RegResult::BadFile: return "BadFile";
  }
  UNREACHABLE();
}
}  // namespace terrain

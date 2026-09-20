#include "indexer/terrain/twm_set.hpp"

#include "indexer/terrain/terrain_serdes.hpp"

#include "base/file_name_utils.hpp"
#include "base/logging.hpp"

namespace terrain
{
std::string DebugPrint(TwmId const & id)
{
  if (id.m_info)
    return "TwmId [" + base::FileNameFromFullPath(id.m_info->GetFilePath()) + "]";
  return "TwmId [invalid]";
}

std::pair<TwmId, TwmSet::RegResult> TwmSet::Register(TwmFile const & file)
{
  ASSERT(!file.m_id.empty(), ());
  ASSERT(file.m_rect.IsValid(), (file.m_rect));
  std::pair<TwmId, RegResult> result;
  WithEventLog([&](EventList & events)
  {
    if (m_condemned.count(file.m_path) > 0)
    {
      result = {TwmId(), RegResult::Condemned};
      return;
    }

    TwmId const existing = GetIdByKeyImpl(file.m_id);
    bool const sameFile = existing.IsAlive() && existing.GetInfo()->GetFilePath() == file.m_path &&
                          existing.GetInfo()->GetVersion() == file.m_version;
    if (existing.IsAlive() && existing.GetInfo()->IsRegistered())
    {
      result = sameFile ? std::make_pair(existing, RegResult::AlreadyRegistered)
                        : std::make_pair(TwmId(), RegResult::Overlapping);
      return;
    }

    // The tracer cannot merge overlapping registered blocks. Marked old blocks only
    // serve outstanding readers and must not prevent their replacements.
    for (auto const & [terrainId, infos] : m_registry)
    {
      if (infos.empty())
        continue;
      auto const & info = infos.back();
      if (info->IsRegistered() && IsInteriorOverlap(file.m_rect, info->GetLimitRect()))
      {
        LOG(LWARNING, ("The terrain file", file.m_path, "overlaps the registered", info->GetFilePath()));
        result = {TwmId(), RegResult::Overlapping};
        return;
      }
    }

    if (sameFile)
    {
      SetStatus(*existing.GetInfo(), TwmInfo::STATUS_REGISTERED, events);
      result = {existing, RegResult::AlreadyRegistered};
      return;
    }

    auto const info = std::make_shared<TwmInfo>(file);
    SetStatus(*info, TwmInfo::STATUS_REGISTERED, events);
    AddToRegistryImpl(info);
    result = {TwmId(info), RegResult::Success};
  });
  return result;
}

bool TwmSet::ReadHeader(std::string const & filePath, TwmHeader & header)
{
  try
  {
    FilesContainerR const container(filePath);
    ReaderSource<ModelReaderPtr> src(container.GetReader(kHeaderTag));
    header.Deserialize(src);
  }
  catch (RootException const & ex)
  {
    LOG(LWARNING, ("Can't read the terrain header", filePath, ":", ex.Msg()));
    return false;
  }
  return true;
}

TwmSet::RegResult TwmSet::ReadFile(std::string const & filePath, int64_t version, TwmFile & file)
{
  TwmHeader header;
  if (!ReadHeader(filePath, header))
    return header.m_version < kTwmVersion ? RegResult::ObsoleteVersion : RegResult::BadFile;
  file = {base::FilenameWithoutExt(base::FileNameFromFullPath(filePath)), version, filePath, header.GetLimitRect()};
  return RegResult::Success;
}

TwmId TwmSet::GetId(TerrainId const & terrainId) const
{
  std::lock_guard<std::mutex> lock(m_lock);
  return GetIdByKeyImpl(terrainId);
}

bool TwmSet::IsFileAlive(TwmFile const & file) const
{
  std::lock_guard<std::mutex> lock(m_lock);
  auto const it = m_registry.find(file.m_id);
  if (it == m_registry.end())
    return false;
  for (auto const & info : it->second)
    if (TwmId(info).IsAlive() && info->GetFilePath() == file.m_path && info->GetVersion() == file.m_version)
      return true;
  return false;
}

bool TwmSet::Deregister(TerrainId const & terrainId)
{
  bool deregistered = false;
  WithEventLog([&](EventList & events)
  {
    TwmId const id = GetIdByKeyImpl(terrainId);
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
  for (auto const & [terrainId, infos] : m_registry)
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
  case TwmInfo::STATUS_REGISTERED: events.Add(Event(Event::TYPE_REGISTERED, info.GetFile())); break;
  case TwmInfo::STATUS_MARKED_TO_DEREGISTER: break;
  case TwmInfo::STATUS_DEREGISTERED: events.Add(Event(Event::TYPE_DEREGISTERED, info.GetFile())); break;
  }
}

void TwmSet::ProcessEvents(EventList & events)
{
  for (auto const & event : events.Get())
  {
    switch (event.m_type)
    {
    case Event::TYPE_REGISTERED: m_observers.ForEach(&Observer::OnTerrainRegistered, event.m_file); break;
    case Event::TYPE_DEREGISTERED: m_observers.ForEach(&Observer::OnTerrainDeregistered, event.m_file); break;
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
  case TwmSet::RegResult::ObsoleteVersion: return "ObsoleteVersion";
  }
  UNREACHABLE();
}
}  // namespace terrain

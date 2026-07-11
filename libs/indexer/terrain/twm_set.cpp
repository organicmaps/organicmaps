#include "indexer/terrain/twm_set.hpp"

#include "indexer/terrain/terrain_serdes.hpp"

#include "geometry/mercator.hpp"

#include "base/file_name_utils.hpp"
#include "base/logging.hpp"

#include <algorithm>

namespace terrain
{
namespace
{
// The blocks share their border lines by design, so only the interiors may not intersect.
bool IsOverlapping(m2::RectD const & lhs, m2::RectD const & rhs)
{
  return lhs.minX() < rhs.maxX() && rhs.minX() < lhs.maxX() && lhs.minY() < rhs.maxY() && rhs.minY() < lhs.maxY();
}

// Splits the possibly beyond-the-antimeridian rect (drape tile rects can poke past +-180,
// see TileKey::GetWrappedDataRect) into the canonical x-range pieces. The registered block
// rects are canonical, so the seam queries must probe both sides.
template <typename Fn>
void ForEachCanonicalRect(m2::RectD const & rect, Fn && fn)
{
  double constexpr kMinX = mercator::Bounds::kMinX;
  double constexpr kMaxX = mercator::Bounds::kMaxX;
  double constexpr kRangeX = kMaxX - kMinX;

  // A wider than the world rect covers every longitude.
  if (rect.SizeX() >= kRangeX)
  {
    fn(m2::RectD(kMinX, rect.minY(), kMaxX, rect.maxY()));
    return;
  }

  m2::RectD canonical = rect;
  if (canonical.Intersect({kMinX, rect.minY(), kMaxX, rect.maxY()}))
    fn(canonical);
  if (rect.maxX() > kMaxX)
    fn(m2::RectD(kMinX, rect.minY(), rect.maxX() - kRangeX, rect.maxY()));
  if (rect.minX() < kMinX)
    fn(m2::RectD(rect.minX() + kRangeX, rect.minY(), kMaxX, rect.maxY()));
}
}  // namespace

std::string DebugPrint(TwmId const & id)
{
  if (id.m_info)
    return "TwmId [" + base::FileNameFromFullPath(id.m_info->GetFilePath()) + "]";
  return "TwmId [invalid]";
}

std::pair<TwmId, TwmSet::RegResult> TwmSet::Register(std::string const & filePath)
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
      LOG(LWARNING, ("Condemning the unreadable terrain file", filePath, ":", ex.Msg()));
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
      if (TwmId(info).IsAlive() && IsOverlapping(limitRect, info->GetLimitRect()))
      {
        LOG(LWARNING, ("The terrain file", filePath, "overlaps the registered", path));
        result = {TwmId(), RegResult::Overlapping};
        return;
      }
    }

    auto const info = std::make_shared<TwmInfo>(filePath, limitRect);
    SetStatus(*info, TwmInfo::STATUS_REGISTERED, events);
    AddToRegistryImpl(info);
    result = {TwmId(info), RegResult::Success};
  });
  return result;
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
    ClearCache(id);
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
      ClearCache(id);
    }
  });
}

template <typename Fn>
void TwmSet::ForEachBlockByRectImpl(m2::RectD const & rect, Fn && fn) const
{
  std::lock_guard<std::mutex> lock(m_lock);
  ForEachCanonicalRect(rect, [&](m2::RectD const & piece)
  {
    for (auto const & [path, infos] : m_registry)
      if (!infos.empty() && infos.back()->IsRegistered() && piece.IsIntersect(infos.back()->GetLimitRect()))
        fn(infos.back());
  });
}

void TwmSet::GetBlocksByRect(m2::RectD const & rect, std::vector<TwmId> & ids) const
{
  ids.clear();
  ForEachBlockByRectImpl(rect, [&](std::shared_ptr<TwmInfo> const & info)
  {
    TwmId const id(info);
    if (std::find(ids.begin(), ids.end(), id) == ids.end())
      ids.push_back(id);
  });
}

bool TwmSet::HasBlocks(m2::RectD const & rect) const
{
  bool found = false;
  ForEachBlockByRectImpl(rect, [&](std::shared_ptr<TwmInfo> const &) { found = true; });
  return found;
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

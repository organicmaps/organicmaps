#include "map/gps_track.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

GpsTrack::GpsTrack(std::string const & filePath, std::unique_ptr<IGpsTrackFilter> && filter)
  : m_filePath(filePath)
  , m_needClear(false)
  , m_needSendSnapshop(false)
  , m_filter(std::move(filter))
  , m_threadExit(false)
  , m_threadWakeup(false)
{
  if (!m_filter)
    m_filter = std::make_unique<GpsTrackNullFilter>();

  ASSERT(!m_filePath.empty(), ());
}

GpsTrack::~GpsTrack()
{
  if (m_thread.joinable())
  {
    {
      std::lock_guard lg(m_threadGuard);
      m_threadExit = true;
      m_cv.notify_one();
    }
    m_thread.join();
  }
}

void GpsTrack::AddPoint(location::GpsInfo const & point)
{
  {
    std::lock_guard lg(m_dataGuard);
    m_points.emplace_back(point);
  }
  ScheduleTask();
}

void GpsTrack::AddPoints(std::vector<location::GpsInfo> const & points)
{
  {
    std::lock_guard lg(m_dataGuard);
    m_points.insert(m_points.end(), points.begin(), points.end());
  }
  ScheduleTask();
}

void GpsTrack::Restore()
{
  ScheduleTask();
}

/// @note These functions are called during recording, so should be synchronized with Collection writer thread.
/// @{
TrackStatistics GpsTrack::GetTrackStatistics() const
{
  std::lock_guard lg(m_threadGuard);
  return m_collection && !m_needClear ? m_collection->GetTrackStatistics() : TrackStatistics();
}

ElevationInfo const & GpsTrack::GetElevationInfo() const
{
  std::lock_guard lg(m_threadGuard);
  CHECK(m_collection, ());
  return m_collection->UpdateAndGetElevationInfo();
}
/// @}

void GpsTrack::Clear()
{
  {
    std::lock_guard threadLock(m_threadGuard);
    std::lock_guard lg(m_dataGuard);
    m_points.clear();
    m_needClear = true;
  }
  ScheduleTask();
}

bool GpsTrack::IsEmpty()
{
  {
    std::lock_guard threadLock(m_threadGuard);
    std::lock_guard dataLock(m_dataGuard);
    if (m_collection && m_points.empty() && !m_needClear)
      return m_collection->IsEmpty();
  }
  ScheduleTask();
  std::unique_lock lock(m_threadGuard);
  // The worker holds this lock until the collection update is complete.
  m_cv.wait(lock, [this] { return !m_threadWakeup; });
  return m_collection->IsEmpty();
}

void GpsTrack::SetCallback(TGpsTrackDiffCallback callback)
{
  bool const subscribed = static_cast<bool>(callback);
  {
    std::lock_guard lg(m_callbackGuard);
    m_callback = std::move(callback);
    m_needSendSnapshop = subscribed;
  }
  if (subscribed)
    ScheduleTask();
}

void GpsTrack::ScheduleTask()
{
  std::lock_guard lg(m_threadGuard);

  /// @todo Replace with !m_thread.joinable() ?
  if (m_thread.get_id() == std::thread::id())
  {
    m_thread = threads::SimpleThread([this]()
    {
      std::unique_lock ul(m_threadGuard);
      while (true)
      {
        m_cv.wait(ul, [this]() { return m_threadExit || m_threadWakeup; });

        if (m_threadWakeup)
        {
          m_threadWakeup = false;
          ProcessPoints();
          m_cv.notify_all();
        }

        if (m_threadExit)
          break;
      }

      m_storage.reset();
    });
  }

  m_threadWakeup = true;
  m_cv.notify_all();
}

void GpsTrack::InitStorageIfNeed()
{
  if (m_storage)
    return;

  try
  {
    m_storage = std::make_unique<GpsTrackStorage>(m_filePath);
  }
  catch (RootException const & e)
  {
    LOG(LWARNING, ("Track storage creation error:", e.Msg()));
  }
}

void GpsTrack::InitCollection()
{
  ASSERT(m_collection == nullptr, ());

  m_collection = std::make_unique<GpsTrackCollection>();

  InitStorageIfNeed();
  if (!m_storage)
    return;

  try
  {
    // All origin points have been written in the storage,
    // and filtered points are inserted in the runtime collection.

    std::vector<location::GpsInfo> originPoints;
    originPoints.reserve(GpsTrackStorage::kItemBlockSize);

    m_storage->ForEach([this, &originPoints](location::GpsInfo const & originPoint) -> bool
    {
      originPoints.emplace_back(originPoint);
      if (originPoints.size() == originPoints.capacity())
      {
        std::vector<location::GpsInfo> points;
        m_filter->Process(originPoints, points);

        m_collection->Add(points);

        originPoints.clear();
      }
      return true;
    });

    if (!originPoints.empty())
    {
      std::vector<location::GpsInfo> points;
      m_filter->Process(originPoints, points);

      m_collection->Add(points);
    }
  }
  catch (RootException const & e)
  {
    LOG(LWARNING, ("Track storage exception:", e.Msg()));
    m_collection->Clear();
    m_storage.reset();
  }
}

void GpsTrack::ProcessPoints()
{
  std::vector<location::GpsInfo> originPoints;
  bool needClear;
  // Steal data for processing
  {
    std::lock_guard lg(m_dataGuard);
    originPoints.swap(m_points);
    needClear = m_needClear;
    m_needClear = false;
  }

  if (!m_collection)
  {
    if (needClear)
      m_collection = std::make_unique<GpsTrackCollection>();
    else
      InitCollection();
  }

  // All origin points are written in the storage,
  // and filtered points are inserted in the runtime collection.

  UpdateStorage(needClear, originPoints);

  if (needClear)
    m_filter->Reset();

  std::vector<location::GpsInfo> points;
  m_filter->Process(originPoints, points);

  std::pair<size_t, size_t> addedIds, evictedIds;
  UpdateCollection(needClear, points, addedIds, evictedIds);

  NotifyCallback(addedIds, evictedIds);
}

size_t GpsTrack::Finalize()
{
  // Saving must process persisted and queued points even if no earlier operation started the worker.
  ScheduleTask();
  {
    std::lock_guard lg(m_threadGuard);
    m_threadWakeup = m_threadExit = true;
    m_cv.notify_all();
  }
  m_thread.join();

  std::lock_guard lock(m_threadGuard);
  m_thread = {};
  m_threadWakeup = m_threadExit = false;

  std::vector<location::GpsInfo> points;
  m_filter->Finalize(points);

  if (!points.empty())
    m_collection->Add(points);

  return m_collection->GetSize();
}

void GpsTrack::UpdateStorage(bool needClear, std::vector<location::GpsInfo> const & points)
{
  InitStorageIfNeed();
  if (!m_storage)
    return;

  try
  {
    if (needClear)
      m_storage->Clear();

    m_storage->Append(points);
  }
  catch (RootException const & e)
  {
    LOG(LWARNING, ("Track storage exception:", e.Msg()));
    m_storage.reset();
  }
}

void GpsTrack::UpdateCollection(bool needClear, std::vector<location::GpsInfo> const & points,
                                std::pair<size_t, size_t> & addedIds, std::pair<size_t, size_t> & evictedIds)
{
  // Apply Clear and Add points
  // Clear points from collection, if need.
  evictedIds = needClear ? m_collection->Clear(false /* resetIds */) : std::make_pair(kInvalidId, kInvalidId);
  ;

  // Add points to the collection, if need
  if (!points.empty())
    addedIds = m_collection->Add(points);
  else
    addedIds = std::make_pair(kInvalidId, kInvalidId);
}

void GpsTrack::NotifyCallback(std::pair<size_t, size_t> const & addedIds, std::pair<size_t, size_t> const & evictedIds)
{
  std::lock_guard lg(m_callbackGuard);

  if (!m_callback)
    return;

  if (m_needSendSnapshop)
  {
    m_needSendSnapshop = false;

    std::vector<std::pair<size_t, location::GpsInfo>> toAdd;
    toAdd.reserve(m_collection->GetSize());
    m_collection->ForEach([&toAdd](location::GpsInfo const & point, size_t id) -> bool
    {
      toAdd.emplace_back(id, point);
      return true;
    });

    if (toAdd.empty())
      return;  // nothing to send

    m_callback(std::move(toAdd), std::make_pair(kInvalidId, kInvalidId), m_collection->GetTrackStatistics());
  }
  else
  {
    std::vector<std::pair<size_t, location::GpsInfo>> toAdd;
    if (addedIds.first != kInvalidId)
    {
      size_t const addedCount = addedIds.second - addedIds.first + 1;
      ASSERT_GREATER_OR_EQUAL(m_collection->GetSize(), addedCount, ());
      toAdd.reserve(addedCount);
      m_collection->ForEach([&toAdd](location::GpsInfo const & point, size_t id) -> bool
      {
        toAdd.emplace_back(id, point);
        return true;
      }, m_collection->GetSize() - addedCount);
      ASSERT_EQUAL(toAdd.size(), addedCount, ());
    }

    if (toAdd.empty() && evictedIds.first == kInvalidId)
      return;  // nothing to send

    m_callback(std::move(toAdd), evictedIds, m_collection->GetTrackStatistics());
  }
}

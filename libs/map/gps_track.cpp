#include "map/gps_track.hpp"

#include "geometry/mercator.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/scope_guard.hpp"

struct GpsTrack::ElevationCache
{
  uint64_t m_epoch;
  size_t m_cursor = 0;
  size_t m_prefixSize = 0;
  double m_distance = 0;
  m2::PointD m_lastPoint;
  TrackElevation m_processor;
  std::shared_ptr<ElevationSnapshot const> m_snapshot;

  explicit ElevationCache(uint64_t epoch) : m_epoch(epoch) {}
};

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

/// @note These functions are called during recording, so should be synchronized with Collection writer thread.
/// @{
TrackStatistics GpsTrack::GetTrackStatistics() const
{
  std::lock_guard lg(m_threadGuard);
  return m_collection && !m_needClear ? m_collection->GetTrackStatistics() : TrackStatistics();
}

ElevationInfo GpsTrack::GetElevationInfo() const
{
  auto const snapshot = GetElevationSnapshot();
  return snapshot ? snapshot->m_elevation : ElevationInfo{};
}
/// @}

std::shared_ptr<GpsTrack::ElevationSnapshot const> GpsTrack::GetElevationSnapshot(IsCancelled const & isCancelled) const
{
  auto const cancelled = [&] { return isCancelled && isCancelled(); };
  if (cancelled())
    return {};
  // The recording worker never takes this lock. Serialize graph requests, not GPS updates.
  std::lock_guard graphLock(m_elevationGuard);
  bool discard = false;
  SCOPE_GUARD(discardOldRecording, [&]
  {
    if (m_elevationCache && (discard || m_elevationCache->m_epoch != m_epoch.load(std::memory_order_relaxed)))
      m_elevationCache.reset();
  });
  uint64_t epoch;
  size_t revision;
  TrackStatistics statistics;
  std::vector<location::GpsInfo> input;
  buffer_vector<TrackElevation::Point, TrackElevation::kMaxTailPoints> tail;
  std::shared_ptr<ElevationSnapshot const> ready;
  {
    std::lock_guard lock(m_threadGuard);
    if (!m_collection || m_needClear || !m_collection->HasAltitudes())
    {
      discard = true;
      return {};
    }
    epoch = m_epoch.load(std::memory_order_relaxed);
    revision = m_collection->GetSize();
    if (m_elevationCache && m_elevationCache->m_epoch == epoch && m_elevationCache->m_cursor == revision)
      ready = m_elevationCache->m_snapshot;
    if (!ready)
    {
      size_t const cursor = m_elevationCache && m_elevationCache->m_epoch == epoch ? m_elevationCache->m_cursor : 0;
      statistics = m_collection->GetTrackStatistics();
      // Statistics already computed this bounded preview. Do not solve it again for the chart.
      auto const preview = m_collection->GetElevationTail();
      tail.assign(preview.begin(), preview.end());
      input.reserve(revision - cursor);
      m_collection->ForEach([&](location::GpsInfo const & point, size_t)
      {
        input.push_back(point);
        return true;
      }, cursor);
    }
  }
  if (cancelled() || m_epoch.load(std::memory_order_relaxed) != epoch)
    return {};
  if (ready)
    return ready;
  if (!m_elevationCache || m_elevationCache->m_epoch != epoch)
    m_elevationCache = std::make_unique<ElevationCache>(epoch);
  auto & cache = *m_elevationCache;
  auto processor = cache.m_processor;
  auto lastPoint = cache.m_lastPoint;
  double distance = cache.m_distance;
  ElevationInfo::Points finalized;
  TrackElevation::Output const output = [&](std::span<TrackElevation::Point const> points)
  { finalized.insert(finalized.end(), points.begin(), points.end()); };
  for (size_t i = 0; i < input.size(); ++i)
  {
    if (i % TrackElevation::kWindowIntervals == 0 && (cancelled() || m_epoch.load(std::memory_order_relaxed) != epoch))
      return {};
    auto const & gps = input[i];
    auto const point = mercator::FromLatLon(gps.m_latitude, gps.m_longitude);
    if (cache.m_cursor + i > 0)
      distance += mercator::DistanceOnEarth(lastPoint, point);
    lastPoint = point;
    processor.Add({distance, static_cast<geometry::Altitude>(gps.m_altitude)}, output);
  }
  if (cancelled() || m_epoch.load(std::memory_order_relaxed) != epoch)
    return {};
  ElevationInfo::Points points;
  points.reserve(cache.m_prefixSize + finalized.size() + tail.size());
  if (cache.m_snapshot)
  {
    auto const & previous = cache.m_snapshot->m_elevation.GetLines().front();
    points.insert(points.end(), previous.begin(), previous.begin() + cache.m_prefixSize);
  }
  points.insert(points.end(), finalized.begin(), finalized.end());
  size_t const prefixSize = points.size();
  points.insert(points.end(), tail.begin(), tail.end());
  auto snapshot = std::make_shared<ElevationSnapshot>(epoch, revision, statistics, ElevationInfo(std::move(points)));
  cache.m_processor = std::move(processor);
  cache.m_lastPoint = lastPoint;
  cache.m_distance = distance;
  cache.m_cursor = revision;
  cache.m_prefixSize = prefixSize;
  cache.m_snapshot = snapshot;
  return snapshot;
}

void GpsTrack::Clear()
{
  {
    std::lock_guard threadLock(m_threadGuard);
    std::lock_guard lg(m_dataGuard);
    m_points.clear();
    m_needClear = true;
    ++m_epoch;
  }
  ScheduleTask();
  // Release a dormant old profile without waiting for an in-flight graph request.
  std::unique_lock graphLock(m_elevationGuard, std::try_to_lock);
  if (graphLock && m_elevationCache && m_elevationCache->m_epoch != m_epoch.load(std::memory_order_relaxed))
    m_elevationCache.reset();
}

bool GpsTrack::IsEmpty() const
{
  std::lock_guard lg(m_threadGuard);
  if (!m_collection || m_needClear)
    return true;
  return m_collection->IsEmpty();
}

void GpsTrack::SetCallback(TGpsTrackDiffCallback callback)
{
  {
    std::lock_guard lg(m_callbackGuard);
    m_callback = callback;
    m_needSendSnapshop = true;
  }
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
        }

        if (m_threadExit)
          break;
      }

      m_storage.reset();
    });
  }

  m_threadWakeup = true;
  m_cv.notify_one();
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

  // Create collection only if callback appears
  if (!m_collection && HasCallback())
  {
    if (needClear)
      m_collection = std::make_unique<GpsTrackCollection>();
    else
      InitCollection();
  }

  // All origin points are written in the storage,
  // and filtered points are inserted in the runtime collection.

  UpdateStorage(needClear, originPoints);

  if (!m_collection)
    return;

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
  if (m_thread.joinable())
  {
    {
      std::lock_guard lg(m_threadGuard);
      m_threadWakeup = m_threadExit = true;
      m_cv.notify_one();
    }
    m_thread.join();

    // Initialize from scratch.
    m_thread = {};
    m_threadWakeup = m_threadExit = false;
  }

  std::lock_guard lock(m_threadGuard);
  std::vector<location::GpsInfo> points;
  m_filter->Finalize(points);

  if (!m_collection)  // If no worker thread was started.
    return 0;

  if (!points.empty())
    m_collection->Add(points);

  return m_collection->GetSize();
}

bool GpsTrack::HasCallback()
{
  std::lock_guard lg(m_callbackGuard);
  return m_callback != nullptr;
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

  StatisticsGetter const getStatistics = [this]() -> TrackStatistics const &
  { return m_collection->GetTrackStatistics(); };

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

    m_callback(std::move(toAdd), std::make_pair(kInvalidId, kInvalidId), getStatistics);
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

    m_callback(std::move(toAdd), evictedIds, getStatistics);
  }
}

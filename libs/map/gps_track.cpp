#include "map/gps_track.hpp"

#include "geometry/mercator.hpp"

#include "base/assert.hpp"
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

GpsTrack::GpsTrack(std::string const & filePath, std::unique_ptr<IGpsTrackFilter> && filter,
                   std::chrono::milliseconds backupInterval)
  : m_backupInterval(backupInterval)
  , m_storage(filePath)
  , m_filter(std::move(filter))
{
  ASSERT(!filePath.empty(), ());
  ASSERT_GREATER(m_backupInterval.count(), 0, ());
  if (!m_filter)
    m_filter = std::make_unique<GpsTrackNullFilter>();
}

GpsTrack::~GpsTrack()
{
  {
    std::lock_guard lg(m_callbackGuard);
    m_callback = nullptr;
    m_statisticsCallback = nullptr;
  }
  Finalize();
}

void GpsTrack::Start()
{
  std::lock_guard lg(m_threadGuard);
  if (m_recording)
    return;

  m_recording = true;
  m_needRestore = true;
  ++m_epoch;
  ScheduleTask();
}

void GpsTrack::AddPoint(location::GpsInfo const & point)
{
  std::lock_guard lg(m_threadGuard);
  if (!m_recording)
    return;
  m_points.push_back(point);
  ScheduleTask();
}

void GpsTrack::AddPoints(std::vector<location::GpsInfo> const & points)
{
  std::lock_guard lg(m_threadGuard);
  if (!m_recording)
    return;
  m_points.insert(m_points.end(), points.begin(), points.end());
  ScheduleTask();
}

TrackStatistics GpsTrack::GetTrackStatistics() const
{
  std::lock_guard lg(m_threadGuard);
  return m_needClear ? TrackStatistics{} : m_collection.GetTrackStatistics();
}

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
    if (m_needClear || m_needRestore || !m_collection.HasAltitudes())
    {
      discard = true;
      return {};
    }
    epoch = m_epoch.load(std::memory_order_relaxed);
    revision = m_collection.GetSize();
    if (m_elevationCache && m_elevationCache->m_epoch == epoch && m_elevationCache->m_cursor == revision)
      ready = m_elevationCache->m_snapshot;
    if (!ready)
    {
      size_t const cursor = m_elevationCache && m_elevationCache->m_epoch == epoch ? m_elevationCache->m_cursor : 0;
      statistics = m_collection.GetTrackStatistics();
      // Statistics already computed this bounded preview. Do not solve it again for the chart.
      auto const preview = m_collection.GetElevationTail();
      tail.assign(preview.begin(), preview.end());
      input.reserve(revision - cursor);
      m_collection.ForEach([&](location::GpsInfo const & point, size_t)
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
    std::lock_guard lg(m_threadGuard);
    m_points.clear();
    m_needClear = true;
    ++m_epoch;
    ScheduleTask();
  }
  // Release a dormant old profile without waiting for an in-flight graph request.
  std::unique_lock graphLock(m_elevationGuard, std::try_to_lock);
  if (graphLock && m_elevationCache && m_elevationCache->m_epoch != m_epoch.load(std::memory_order_relaxed))
    m_elevationCache.reset();
}

bool GpsTrack::IsEmpty() const
{
  std::lock_guard lg(m_threadGuard);
  return m_needClear || m_collection.IsEmpty();
}

void GpsTrack::SetCallback(TGpsTrackDiffCallback callback)
{
  {
    std::lock_guard lg(m_callbackGuard);
    m_callback = std::move(callback);
    m_needSendSnapshot = true;
  }
  std::lock_guard lg(m_threadGuard);
  ScheduleTask();
}

void GpsTrack::SetStatisticsCallback(TGpsTrackStatisticsCallback callback)
{
  {
    std::lock_guard lg(m_callbackGuard);
    m_statisticsCallback = std::move(callback);
    m_needSendStatistics = true;
  }
  std::lock_guard lg(m_threadGuard);
  ScheduleTask();
}

void GpsTrack::ScheduleTask()
{
  if (!m_thread.joinable())
  {
    m_thread = threads::SimpleThread([this]()
    {
      std::unique_lock ul(m_threadGuard);
      while (true)
      {
        auto const hasTask = [this]() { return m_threadExit || m_threadWakeup; };
        if (m_backupPoints.empty())
          m_cv.wait(ul, hasTask);
        else
          m_cv.wait_until(ul, m_nextBackup, hasTask);

        if (m_threadWakeup)
        {
          m_threadWakeup = false;
          ProcessPoints();
        }

        if (m_threadExit)
          break;

        if (!m_backupPoints.empty() && Clock::now() >= m_nextBackup)
          SaveBackup();
      }
    });
  }

  m_threadWakeup = true;
  m_cv.notify_one();
}

void GpsTrack::ProcessPoints()
{
  auto evictedIds = std::make_pair(kInvalidId, kInvalidId);
  std::vector<location::GpsInfo> points;
  bool const needReset = m_needClear || m_needRestore;
  if (needReset)
  {
    evictedIds = m_collection.Clear(false /* resetIds */);
    m_filter->Reset();
    m_backupPoints.clear();
    if (m_needClear)
      ClearBackup();
    else
      m_filter->Process(m_storage.Load(), points);
    m_needClear = m_needRestore = false;
  }

  m_filter->Process(m_points, points);
  auto const addedIds = m_collection.Add(points);

  if (m_backupPoints.empty())
    m_nextBackup = Clock::now() + m_backupInterval;
  m_backupPoints.insert(m_backupPoints.end(), m_points.begin(), m_points.end());
  m_points.clear();

  NotifyCallback(addedIds, evictedIds);
  NotifyStatisticsCallback(needReset || addedIds.first != kInvalidId);
}

void GpsTrack::SaveBackup()
{
  if (m_backupPoints.empty())
    return;
  if (m_storage.Append(m_backupPoints))
    m_backupPoints.clear();
  else
    m_nextBackup = Clock::now() + m_backupInterval;
}

void GpsTrack::ClearBackup()
{
  m_backupPoints.clear();
  m_storage.Clear();
}

size_t GpsTrack::Finalize()
{
  std::unique_lock ul(m_threadGuard);
  m_recording = false;
  if (m_thread.joinable())
  {
    m_threadWakeup = m_threadExit = true;
    m_cv.notify_one();
    ul.unlock();
    m_thread.join();
    ul.lock();

    m_thread = {};
    m_threadWakeup = m_threadExit = false;
  }

  SaveBackup();
  std::vector<location::GpsInfo> points;
  m_filter->Finalize(points);
  m_collection.Add(points);
  return m_collection.GetSize();
}

void GpsTrack::NotifyCallback(std::pair<size_t, size_t> const & addedIds, std::pair<size_t, size_t> const & evictedIds)
{
  std::lock_guard lg(m_callbackGuard);
  if (!m_callback)
    return;

  size_t addedCount = 0;
  auto toRemove = evictedIds;
  if (m_needSendSnapshot)
  {
    m_needSendSnapshot = false;
    addedCount = m_collection.GetSize();
    toRemove = std::make_pair(kInvalidId, kInvalidId);
  }
  else if (addedIds.first != kInvalidId)
    addedCount = addedIds.second - addedIds.first + 1;

  if (addedCount == 0 && toRemove.first == kInvalidId)
    return;

  ASSERT_LESS_OR_EQUAL(addedCount, m_collection.GetSize(), ());
  std::vector<std::pair<size_t, location::GpsInfo>> toAdd;
  toAdd.reserve(addedCount);
  m_collection.ForEach([&toAdd](location::GpsInfo const & point, size_t id)
  {
    toAdd.emplace_back(id, point);
    return true;
  }, m_collection.GetSize() - addedCount);
  m_callback(std::move(toAdd), toRemove);
}

void GpsTrack::NotifyStatisticsCallback(bool changed)
{
  std::lock_guard lg(m_callbackGuard);
  if (m_statisticsCallback && (changed || m_needSendStatistics))
    m_statisticsCallback(m_collection.GetTrackStatistics());
  m_needSendStatistics = false;
}

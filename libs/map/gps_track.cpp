#include "map/gps_track.hpp"

#include "base/assert.hpp"

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
  return m_collection.GetTrackStatistics();
}

ElevationInfo const & GpsTrack::GetElevationInfo()
{
  std::lock_guard lg(m_threadGuard);
  return m_collection.UpdateAndGetElevationInfo();
}

void GpsTrack::Clear()
{
  std::lock_guard lg(m_threadGuard);
  m_points.clear();
  m_needClear = true;
  ScheduleTask();
}

bool GpsTrack::IsEmpty() const
{
  std::lock_guard lg(m_threadGuard);
  return m_collection.IsEmpty();
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

#include "map/gps_track.hpp"

#include "coding/file_name_utils.hpp"

#include "platform/platform.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

#include "defines.hpp"

namespace
{

size_t const kMaxItemCount = 100000;
hours const kDefaultDuration = hours(24);

inline pair<size_t, size_t> UnionRanges(pair<size_t, size_t> const & a, pair<size_t, size_t> const & b)
{
  if (a.first == GpsTrack::kInvalidId)
  {
    ASSERT_EQUAL(a.second, GpsTrack::kInvalidId, ());
    return b;
  }
  if (b.first == GpsTrack::kInvalidId)
  {
    ASSERT_EQUAL(b.second, GpsTrack::kInvalidId, ());
    return a;
  }
  ASSERT_LESS_OR_EQUAL(a.first, a.second, ());
  ASSERT_LESS_OR_EQUAL(b.first, b.second, ());
  return make_pair(min(a.first, b.first), max(a.second, b.second));
}

} // namespace

size_t const GpsTrack::kInvalidId = GpsTrackCollection::kInvalidId;

GpsTrack::GpsTrack(string const & filePath, size_t maxItemCount, hours duration)
  : m_maxItemCount(maxItemCount)
  , m_filePath(filePath)
  , m_duration(duration)
  , m_needClear(false)
  , m_needSendSnapshop(false)
  , m_threadExit(false)
  , m_threadWakeup(false)
{
  ASSERT_GREATER(m_maxItemCount, 0, ());
  ASSERT(!m_filePath.empty(), ());
  ASSERT_GREATER(m_duration.count(), 0, ());
}

GpsTrack::~GpsTrack()
{
  if (m_thread.joinable())
  {
    {
      lock_guard<mutex> lg(m_threadGuard);
      m_threadExit = true;
      m_cv.notify_one();
    }
    m_thread.join();
  }
}

void GpsTrack::AddPoint(TItem const & point)
{
  {
    lock_guard<mutex> lg(m_dataGuard);
    m_points.emplace_back(point);
  }
  ScheduleTask();
}

void GpsTrack::AddPoints(vector<TItem> const & points)
{
  {
    lock_guard<mutex> lg(m_dataGuard);
    m_points.insert(m_points.end(), points.begin(), points.end());
  }
  ScheduleTask();
}

void GpsTrack::Clear()
{
  {
    lock_guard<mutex> lg(m_dataGuard);
    m_points.clear();
    m_needClear = true;
  }
  ScheduleTask();
}

void GpsTrack::SetDuration(hours duration)
{
  ASSERT_GREATER(duration.count(), 0, ());

  {
    lock_guard<mutex> lg(m_dataGuard);
    if (m_duration == duration)
      return;
    m_duration = duration;
  }
  if (HasCallback())
    ScheduleTask();
}

hours GpsTrack::GetDuration() const
{
  lock_guard<mutex> lg(m_dataGuard);
  return m_duration;
}

void GpsTrack::SetCallback(TGpsTrackDiffCallback callback)
{
  {
    lock_guard<mutex> lg(m_callbackGuard);
    m_callback = callback;
    m_needSendSnapshop = true;
  }
  ScheduleTask();
}

void GpsTrack::ScheduleTask()
{
  lock_guard<mutex> lg(m_threadGuard);

  if (m_thread.get_id() == thread::id())
  {
    m_thread = thread([this]()
    {
      unique_lock<mutex> ul(m_threadGuard);
      while (true)
      {
        m_cv.wait(ul, [this]()->bool{ return m_threadExit || m_threadWakeup; });
        if (m_threadExit)
          break;
        m_threadWakeup = false;
        ProcessPoints();
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
    m_storage = make_unique<GpsTrackStorage>(m_filePath, m_maxItemCount);
  }
  catch (RootException & e)
  {
    LOG(LINFO, ("Storage has not been created:", e.Msg()));
  }
}

void GpsTrack::InitCollection(hours duration)
{
  ASSERT(m_collection == nullptr, ());

  m_collection = make_unique<GpsTrackCollection>(m_maxItemCount, duration);

  InitStorageIfNeed();
  if (!m_storage)
    return;

  try
  {
    m_storage->ForEach([this](TItem const & info)->bool
    {
      pair<size_t, size_t> evictedIds;
      m_collection->Add(info, evictedIds);
      return true;
    });
  }
  catch (RootException & e)
  {
    LOG(LINFO, ("Storage has caused exception:", e.Msg()));
    m_collection->Clear();
    m_storage.reset();
  }
}

void GpsTrack::ProcessPoints()
{
  vector<TItem> points;
  hours duration;
  bool needClear;
  // Steal data for processing
  {
    lock_guard<mutex> lg(m_dataGuard);
    points.swap(m_points);
    duration = m_duration;
    needClear = m_needClear;
    m_needClear = false;
  }

  // Create collection only if callback appears
  if (!m_collection && HasCallback())
    InitCollection(duration);

  UpdateStorage(needClear, points);

  if (!m_collection)
    return;

  UpdateCollection(duration, needClear, points);
}

bool GpsTrack::HasCallback()
{
  lock_guard<mutex> lg(m_callbackGuard);
  return m_callback != nullptr;
}

void GpsTrack::UpdateStorage(bool needClear, vector<TItem> const & points)
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
  catch (RootException & e)
  {
    LOG(LINFO, ("Storage has caused exception:", e.Msg()));
    m_storage.reset();
  }
}

void GpsTrack::UpdateCollection(hours duration, bool needClear, vector<TItem> const & points)
{
  // Apply Clear, SetDuration and Add points

  // Clear points from collection, if need.
  pair<size_t, size_t> evictedIdsByClear = make_pair(kInvalidId, kInvalidId);
  if (needClear)
    evictedIdsByClear = m_collection->Clear(false /* resetIds */);

  // Set duration for collection, if need
  // Set duration before Add because new duration can be more than previous value.
  pair<size_t, size_t> evictedIdsByDuration = make_pair(kInvalidId, kInvalidId);
  if (duration != m_collection->GetDuration())
    evictedIdsByDuration = m_collection->SetDuration(duration);

  // Add points to the collection, if need
  pair<size_t, size_t> evictedIds = make_pair(kInvalidId, kInvalidId);
  pair<size_t, size_t> addedIds = make_pair(kInvalidId, kInvalidId);
  if (!points.empty())
    addedIds = m_collection->Add(points, evictedIds);

  // Result evicted is
  evictedIds = UnionRanges(evictedIds, UnionRanges(evictedIdsByClear, evictedIdsByDuration));

  // Send callback notification.
  // Callback must be protected by m_callbackGuard

  lock_guard<mutex> lg(m_callbackGuard);

  if (!m_callback)
    return;

  if (m_needSendSnapshop)
  {
    m_needSendSnapshop = false;

    // Get all points from collection to send them to the callback
    vector<pair<size_t, TItem>> toAdd;
    toAdd.reserve(m_collection->GetSize());
    m_collection->ForEach([&toAdd](TItem const & point, size_t id)->bool
    {
      toAdd.emplace_back(id, point);
      return true;
    });

    if (toAdd.empty())
      return; // nothing to send

    m_callback(move(toAdd), make_pair(kInvalidId, kInvalidId));
  }
  else
  {
    vector<pair<size_t, TItem>> toAdd;
    if (addedIds.first != kInvalidId)
    {
      size_t const addedCount = addedIds.second - addedIds.first + 1;
      ASSERT_GREATER_OR_EQUAL(m_collection->GetSize(), addedCount, ());

      // Not all points from infos could be added to collection due to timestamp consequence restriction.
      // Get added points from collection - take last <addedCount> points from collection, these points
      // were added this time.
      toAdd.reserve(addedCount);
      m_collection->ForEach([&toAdd](TItem const & point, size_t id)->bool
      {
        toAdd.emplace_back(id, point);
        return true;
      }, m_collection->GetSize() - addedCount);
      ASSERT_EQUAL(toAdd.size(), addedCount, ());
    }

    if (toAdd.empty() && evictedIds.first == kInvalidId)
      return; // nothing to send

    m_callback(move(toAdd), evictedIds);
  }
}

GpsTrack & GetDefaultGpsTrack()
{
  static GpsTrack instance(my::JoinFoldersToPath(GetPlatform().WritableDir(), GPS_TRACK_FILENAME), kMaxItemCount, kDefaultDuration);
  return instance;
}

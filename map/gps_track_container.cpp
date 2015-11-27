#include "map/gps_track_container.hpp"

#include "base/logging.hpp"

namespace
{

uint32_t constexpr kDefaultMaxSize = 100000;
hours constexpr kDefaultDuration = hours(24);

uint32_t constexpr kSecondsPerHour = 60 * 60;

} // namespace

GpsTrackContainer::GpsTrackContainer()
    : m_trackDuration(kDefaultDuration)
    , m_maxSize(kDefaultMaxSize)
    , m_counter(0)
{
}

void GpsTrackContainer::SetDuration(hours duration)
{
  lock_guard<mutex> lg(m_guard);

  m_trackDuration = duration;

  vector<uint32_t> removed;
  RemoveOldPoints(removed);

  if (m_callback && !removed.empty())
    m_callback(vector<GpsTrackPoint>(), move(removed));
}

void GpsTrackContainer::SetMaxSize(size_t maxSize)
{
  lock_guard<mutex> lg(m_guard);

  m_maxSize = maxSize;

  vector<uint32_t> removed;
  RemoveOldPoints(removed);

  if (m_callback && !removed.empty())
    m_callback(vector<GpsTrackPoint>(), move(removed));
}

void GpsTrackContainer::SetCallback(TGpsTrackDiffCallback callback, bool sendAll)
{
  lock_guard<mutex> lg(m_guard);

  m_callback = callback;

  if (!m_callback || !sendAll || m_points.empty())
    return;

  vector<GpsTrackPoint> added;
  CopyPoints(added);

  m_callback(move(added), vector<uint32_t>());
}

uint32_t GpsTrackContainer::AddPoint(m2::PointD const & point, double speedMPS, double timestamp)
{
  lock_guard<mutex> lg(m_guard);

  // Do not process points which are come with timestamp earlier than timestamp of the last point
  // because it is probably some error in logic or gps error, because valid gps must provide UTC time which is growing.
  if (!m_points.empty() && timestamp < m_points.back().m_timestamp)
  {
    LOG(LINFO, ("Incorrect GPS timestamp sequence"));
    return kInvalidId;
  }

  GpsTrackPoint gtp;
  gtp.m_timestamp = timestamp;
  gtp.m_point = point;
  gtp.m_speedMPS = speedMPS;
  gtp.m_id = ++m_counter;

  m_points.push_back(gtp);

  vector<GpsTrackPoint> added;
  added.emplace_back(gtp);

  vector<uint32_t> removed;
  RemoveOldPoints(removed);

  if (m_callback)
    m_callback(move(added), move(removed));

  return gtp.m_id;
}

void GpsTrackContainer::GetPoints(vector<GpsTrackPoint> & points) const
{
  lock_guard<mutex> lg(m_guard);

  CopyPoints(points);
}

void GpsTrackContainer::RemoveOldPoints(vector<uint32_t> & removedIds)
{
  // Must be called under m_guard lock

  if (m_points.empty())
    return;

  time_t const lowerBorder = m_points.back().m_timestamp - kSecondsPerHour * m_trackDuration.count();

  if (m_points.front().m_timestamp < lowerBorder)
  {
    GpsTrackPoint pt;
    pt.m_timestamp = lowerBorder;

    auto const itr = lower_bound(m_points.begin(), m_points.end(), pt,
                                 [](GpsTrackPoint const & a, GpsTrackPoint const & b)->bool{ return a.m_timestamp < b.m_timestamp; });

    if (itr != m_points.begin())
    {
      removedIds.reserve(removedIds.size() + distance(m_points.begin(), itr));
      for (auto i = m_points.begin(); i != itr; ++i)
        removedIds.emplace_back(i->m_id);

      m_points.erase(m_points.begin(), itr);
    }
  }

  if (m_points.size() > m_maxSize)
  {
    auto const itr = m_points.begin() + m_points.size() - m_maxSize;

    removedIds.reserve(removedIds.size() + distance(m_points.begin(), itr));
    for (auto i = m_points.begin(); i != itr; ++i)
      removedIds.emplace_back(i->m_id);

    m_points.erase(m_points.begin(), itr);
  }
}

void GpsTrackContainer::CopyPoints(vector<GpsTrackPoint> & points) const
{
  // Must be called under m_guard lock

  vector<GpsTrackPoint> tmp;
  tmp.reserve(m_points.size());
  copy(m_points.begin(), m_points.end(), back_inserter(tmp));
  points.swap(tmp);
}

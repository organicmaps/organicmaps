#pragma once

#include "geometry/rect2d.hpp"
#include "geometry/point2d.hpp"

#include "base/assert.hpp"
#include "base/non_intersecting_intervals.hpp"

#include <atomic>
#include <limits>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace poly_borders
{
struct AtomicBoolWrapper
{
  AtomicBoolWrapper() { m_value = false; }
  AtomicBoolWrapper(bool value) { m_value = value; }
  AtomicBoolWrapper(std::atomic<bool> const & rhs) { m_value = rhs.load(); }
  AtomicBoolWrapper(AtomicBoolWrapper const & rhs) { m_value = rhs.m_value.load(); }
  AtomicBoolWrapper operator=(AtomicBoolWrapper const & rhs)
  {
    m_value = rhs.m_value.load();
    return *this;
  }

  explicit operator bool() const { return m_value.load(); }

  std::atomic<bool> m_value;
};

struct Link
{
  inline static auto constexpr kInvalidId = std::numeric_limits<size_t>::max();

  Link() = default;
  Link(size_t borderId, size_t pointId) : m_borderId(borderId), m_pointId(pointId) {}

  bool operator<(Link const & rhs) const;

  size_t m_borderId = kInvalidId;
  size_t m_pointId = kInvalidId;
};

/// \note Using next semantic here: [replaceFrom, replaceTo], [replaceFromSrc, replaceToSrc].
struct ReplaceData
{
  ReplaceData(size_t replaceFrom, size_t replaceTo, size_t replaceFromSrc, size_t replaceToSrc,
              size_t borderIdSrc, bool reversed)
    : m_dstFrom(replaceFrom)
    , m_dstTo(replaceTo)
    , m_srcReplaceFrom(replaceFromSrc)
    , m_srcReplaceTo(replaceToSrc)
    , m_srcBorderId(borderIdSrc)
    , m_reversed(reversed) {}

  bool operator<(ReplaceData const & rhs) const;

  size_t m_dstFrom;
  size_t m_dstTo;

  size_t m_srcReplaceFrom;
  size_t m_srcReplaceTo;
  size_t m_srcBorderId;

  // If |m_srcReplaceFrom| was greater than |m_srcReplaceTo|.
  bool m_reversed;
};

struct MarkedPoint
{
  MarkedPoint() = default;
  explicit MarkedPoint(m2::PointD const & point) : m_point(point) {}

  void AddLink(size_t borderId, size_t pointId);

  std::optional<Link> GetLink(size_t curBorderId) const;

  m2::PointD m_point;
  AtomicBoolWrapper m_marked;
  std::set<Link> m_links;
  std::unique_ptr<std::mutex> m_mutex = std::make_unique<std::mutex>();
};

struct Polygon
{
  Polygon() = default;
  Polygon(m2::RectD const & rect, std::vector<MarkedPoint> && points)
    : m_rect(rect), m_points(std::move(points)) {}

  // [a, b]
  // @{
  void MakeFrozen(size_t a, size_t b);
  bool IsFrozen(size_t a, size_t b) const;
  // @}

  // [replaceFrom, replaceTo], [replaceFromSrc, replaceToSrc]
  void AddReplaceInfo(size_t replaceFrom, size_t replaceTo,
                      size_t replaceFromSrc, size_t replaceToSrc, size_t borderIdSrc,
                      bool reversed);

  std::set<ReplaceData>::const_iterator FindReplaceData(size_t index);

  m2::RectD m_rect;
  std::vector<MarkedPoint> m_points;
  base::NonIntersectingIntervals<size_t> m_replaced;
  std::set<ReplaceData> m_replaceData;
};
}  // namespace poly_borders

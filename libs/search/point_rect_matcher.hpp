#pragma once

#include "search/segment_tree.hpp"

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <sstream>
#include <string>
#include <vector>

namespace search
{
class PointRectMatcher
{
public:
  enum class RequestType : uint8_t
  {
    // A request to match every point to an arbitrary rectangle that contains it.
    Any,

    // A request to match every point to every rectangle that contains it.
    // Every relevant (point, rectangle) pair must be listed exactly once.
    All
  };

  struct PointIdPair
  {
    PointIdPair() = default;
    PointIdPair(m2::PointD const & point, size_t id) : m_point(point), m_id(id) {}
    m2::PointD m_point = m2::PointD::Zero();
    size_t m_id = 0;
  };

  struct RectIdPair
  {
    RectIdPair() = default;
    RectIdPair(m2::RectD const & rect, size_t id) : m_rect(rect), m_id(id) {}
    m2::RectD m_rect;
    size_t m_id = 0;
  };

  // For each point tries to find a rect containing the point. If
  // there are several rects containing the point, selects an
  // arbitrary one. Calls |fn| on matched pairs (point-id, rect-id).
  //
  // Complexity: O(n * log(n)), where n = |points| + |rects|.
  template <typename Fn>
  static void Match(std::vector<PointIdPair> const & points, std::vector<RectIdPair> const & rects,
                    RequestType requestType, Fn && fn)
  {
    std::vector<Event> events;
    events.reserve(points.size() + 2 * rects.size());

    for (auto const & p : points)
      events.emplace_back(PointEvent(p.m_point.x, p.m_id), p.m_point.y);

    std::vector<SegmentTree::Segment> segments;
    segments.reserve(rects.size());

    for (auto const & r : rects)
    {
      SegmentTree::Segment const segment(r.m_rect.minX(), r.m_rect.maxX(), r.m_id);

      segments.push_back(segment);
      events.emplace_back(Event::TYPE_SEGMENT_START, segment, r.m_rect.minY());
      events.emplace_back(Event::TYPE_SEGMENT_END, segment, r.m_rect.maxY());
    }

    std::sort(segments.begin(), segments.end());
    SegmentTree tree(segments);

    std::sort(events.begin(), events.end());
    for (auto const & e : events)
    {
      switch (e.m_type)
      {
      case Event::TYPE_SEGMENT_START: tree.Add(e.m_segment); break;
      case Event::TYPE_POINT:
      {
        auto const segmentFn = [&](SegmentTree::Segment const & segment) { fn(e.m_point.m_id, segment.m_id); };
        switch (requestType)
        {
        case RequestType::Any: tree.FindAny(e.m_point.m_x, segmentFn); break;
        case RequestType::All: tree.FindAll(e.m_point.m_x, segmentFn); break;
        }
      }
      break;
      case Event::TYPE_SEGMENT_END: tree.Erase(e.m_segment); break;
      }
    }
  }

private:
  struct PointEvent
  {
    PointEvent() = default;
    PointEvent(double x, size_t id) : m_x(x), m_id(id) {}
    double m_x = 0;
    size_t m_id = 0;
  };

  using SegmentEvent = SegmentTree::Segment;

  struct Event
  {
    enum Type
    {
      TYPE_SEGMENT_START,
      TYPE_POINT,
      TYPE_SEGMENT_END,
    };

    Event(Type type, SegmentEvent const & segment, double time) : m_segment(segment), m_time(time), m_type(type)
    {
      ASSERT(m_type == TYPE_SEGMENT_START || m_type == TYPE_SEGMENT_END, ());
    }

    Event(PointEvent const & point, double time) : m_point(point), m_time(time), m_type(TYPE_POINT) {}

    bool operator<(Event const & rhs) const
    {
      if (m_time != rhs.m_time)
        return m_time < rhs.m_time;
      return m_type < rhs.m_type;
    }

    union
    {
      SegmentEvent m_segment;
      PointEvent m_point;
    };

    double m_time;
    Type m_type;
  };
};
}  // namespace search

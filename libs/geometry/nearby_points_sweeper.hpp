#pragma once

#include "base/assert.hpp"
#include "base/visitor.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <set>
#include <utility>
#include <vector>

namespace m2
{
// This class can be used to greedily sweep points on a plane that are
// too close to each other.  Two points are considered to be "too
// close" when distance along axes between them is less than or equal
// to some preselected epsilon. Different epsilons can be used for different axes.
// Note, the result is not the largest subset of points that can be selected,
// but it can be computed quite fast and gives satisfactory results.
//
// *NOTE* The class is NOT thread-safe.
class NearbyPointsSweeper
{
public:
  explicit NearbyPointsSweeper(double eps);

  NearbyPointsSweeper(double xEps, double yEps);

  // Adds a new point (|x|, |y|) on the plane. |index| is used to
  // identify individual points, and will be reported for survived
  // points during the Sweep phase.
  // Points with higher |priority| will be preferred during the Sweep phase.
  void Add(double x, double y, size_t index, uint8_t priority);

  // Emits indexes of all survived points. Complexity: O(n * log(n)),
  // where n is the number of already added points.
  template <typename TEmitter>
  void Sweep(TEmitter && emitter)
  {
    std::sort(m_events.begin(), m_events.end());

    std::set<LineEvent> line;

    for (auto const & event : m_events)
    {
      switch (event.m_type)
      {
      case Event::TYPE_SEGMENT_START:
      {
        if (line.empty())
        {
          line.insert({event.m_x, event.m_index, event.m_priority});
          break;
        }

        auto it =
            line.upper_bound({event.m_x, std::numeric_limits<size_t>::max(), std::numeric_limits<uint8_t>::max()});

        bool add = true;
        while (true)
        {
          if (line.empty())
            break;

          if (it == line.end())
          {
            --it;
            continue;
          }

          double const x = it->m_x;
          if (fabs(x - event.m_x) <= m_xEps)
          {
            if (it->m_priority >= event.m_priority)
            {
              add = false;
              break;
            }
            // New event has higher priority than an existing event nearby.
            it = line.erase(it);
          }

          if (x + m_xEps < event.m_x)
            break;

          if (it == line.begin())
            break;

          --it;
        }

        if (add)
          line.insert({event.m_x, event.m_index, event.m_priority});

        break;
      }

      case Event::TYPE_SEGMENT_END:
      {
        auto it = line.find({event.m_x, event.m_index, event.m_priority});
        if (it != line.end())
        {
          emitter(event.m_index);
          line.erase(it);
        }
        break;
      }
      };
    }

    ASSERT(line.empty(), ("Sweep line must be empty after events processing:", line));
  }

private:
  struct Event
  {
    enum Type
    {
      TYPE_SEGMENT_START,
      TYPE_SEGMENT_END
    };

    Event(Type type, double y, double x, size_t index, uint8_t priority);

    bool operator<(Event const & rhs) const;

    Type m_type;

    double m_y;
    double m_x;
    size_t m_index;
    uint8_t m_priority;
  };

  struct LineEvent
  {
    DECLARE_VISITOR_AND_DEBUG_PRINT(LineEvent, visitor(m_x, "x"), visitor(m_index, "index"),
                                    visitor(m_priority, "priority"))
    bool operator<(LineEvent const & rhs) const
    {
      if (m_x != rhs.m_x)
        return m_x < rhs.m_x;
      if (m_index != rhs.m_index)
        return m_index < rhs.m_index;
      return m_priority < rhs.m_priority;
    }

    double m_x;
    size_t m_index;
    uint8_t m_priority;
  };

  std::vector<Event> m_events;
  double const m_xEps;
  double const m_yEps;
};
}  // namespace m2

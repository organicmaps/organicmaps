#pragma once

#include "generator/intermediate_data.hpp"
#include "generator/intermediate_elements.hpp"

#include "geometry/point2d.hpp"

#include <cstdint>
#include <map>
#include <memory>
#include <utility>
#include <vector>

namespace generator
{
class AreaWayMerger
{
  using PointSeq = std::vector<m2::PointD>;
  using WayMap = std::multimap<uint64_t, std::shared_ptr<WayElement>>;
  using WayMapIterator = WayMap::iterator;

public:
  explicit AreaWayMerger(cache::IntermediateDataReader & holder);

  void AddWay(uint64_t id);

  template <class ToDo>
  void ForEachArea(bool collectID, ToDo toDo)
  {
    while (!m_map.empty())
    {
      // start
      WayMapIterator i = m_map.begin();
      uint64_t id = i->first;

      std::vector<uint64_t> ids;
      PointSeq points;

      do
      {
        // process way points
        std::shared_ptr<WayElement> e = i->second;
        if (collectID)
          ids.push_back(e->m_wayOsmId);

        e->ForEachPointOrdered(id, [this, &points](uint64_t id)
        {
          m2::PointD pt;
          if (m_holder.GetNode(id, pt.y, pt.x))
            points.push_back(pt);
        });

        m_map.erase(i);

        // next 'id' to process
        id = e->GetOtherEndPoint(id);
        std::pair<WayMapIterator, WayMapIterator> r = m_map.equal_range(id);

        // finally erase element 'e' and find next way in chain
        i = r.second;
        while (r.first != r.second)
        {
          if (r.first->second == e)
            m_map.erase(r.first++);
          else
            i = r.first++;
        }

        if (i == r.second)
          break;
      } while (true);

      if (points.size() > 2 && points.front() == points.back())
        toDo(points, ids);
    }
  }

private:
  cache::IntermediateDataReader & m_holder;
  WayMap m_map;
};
}  // namespace generator

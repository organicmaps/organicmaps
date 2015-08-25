#pragma once
#include "generator/intermediate_elements.hpp"

#include "geometry/point2d.hpp"

#include "std/map.hpp"
#include "std/vector.hpp"
#include "std/shared_ptr.hpp"


template <class THolder>
class AreaWayMerger
{
  THolder & m_holder;

  typedef vector<m2::PointD> pts_vec_t;

  static bool IsValidAreaPath(pts_vec_t const & pts)
  {
    return (pts.size() > 2 && pts.front() == pts.back());
  }

  bool GetPoint(uint64_t id, m2::PointD & pt) const
  {
    return m_holder.GetNode(id, pt.y, pt.x);
  }

  class process_points
  {
    AreaWayMerger * m_pMain;

  public:
    pts_vec_t m_vec;

    process_points(AreaWayMerger * pMain) : m_pMain(pMain) {}
    void operator()(uint64_t id)
    {
      m2::PointD pt;
      if (m_pMain->GetPoint(id, pt))
        m_vec.push_back(pt);
    }
  };

  typedef multimap<uint64_t, shared_ptr<WayElement> > way_map_t;
  way_map_t m_map;

public:
  AreaWayMerger(THolder & holder) : m_holder(holder) {}

  void AddWay(uint64_t id)
  {
    shared_ptr<WayElement> e(new WayElement(id));
    if (m_holder.GetWay(id, *e) && e->IsValid())
    {
      m_map.insert(make_pair(e->nodes.front(), e));
      m_map.insert(make_pair(e->nodes.back(), e));
    }
  }

  template <class ToDo>
  void ForEachArea(ToDo & toDo, bool collectID)
  {
    while (!m_map.empty())
    {
      typedef way_map_t::iterator iter_t;

      // start
      iter_t i = m_map.begin();
      uint64_t id = i->first;

      process_points process(this);
      vector<uint64_t> ids;

      do
      {
        // process way points
        shared_ptr<WayElement> e = i->second;
        if (collectID)
          ids.push_back(e->m_wayOsmId);
        e->ForEachPointOrdered(id, process);

        m_map.erase(i);

        // next 'id' to process
        id = e->GetOtherEndPoint(id);
        pair<iter_t, iter_t> r = m_map.equal_range(id);

        // finally erase element 'e' and find next way in chain
        i = r.second;
        while (r.first != r.second)
        {
          if (r.first->second == e)
            m_map.erase(r.first++);
          else
          {
            i = r.first;
            ++r.first;
          }
        }

        if (i == r.second) break;
      } while (true);

      if (IsValidAreaPath(process.m_vec))
        toDo(process.m_vec, ids);
    }
  }
};

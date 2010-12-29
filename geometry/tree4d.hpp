#pragma once

#include "rect2d.hpp"
#include "point2d.hpp"

#include "../base/stl_add.hpp"

#include "../std/kdtree.hpp"


namespace m4
{
  template <class T>
  class Tree
  {
    struct value_t
    {
      T m_val;
      double m_pts[4];

      typedef double value_type;

      value_t(T const & t, m2::RectD const & r) : m_val(t)
      {
        m_pts[0] = r.minX();
        m_pts[1] = r.minY();
        m_pts[2] = r.maxX();
        m_pts[3] = r.maxY();
      }

      bool IsIntersect(m2::RectD const & r) const
      {
        return !((m_pts[2] <= r.minX()) || (m_pts[0] >= r.maxX()) ||
                 (m_pts[3] <= r.minY()) || (m_pts[1] >= r.maxY()));
      }

      double operator[](size_t i) const { return m_pts[i]; }
    };

    typedef KDTree::KDTree<4, value_t> tree_t;
    typedef typename tree_t::_Region_ region_t;
    tree_t m_tree;

    typedef vector<value_t const *> store_vec_t;

    class insert_if_intersect
    {
      store_vec_t & m_isect;
      m2::RectD const & m_rect;

    public:
      insert_if_intersect(store_vec_t & isect, m2::RectD const & r)
        : m_isect(isect), m_rect(r)
      {
      }

      void operator() (value_t const & v)
      {
        if (v.IsIntersect(m_rect))
          m_isect.push_back(&v);
      }
    };

  public:

    template <class TCompare>
    void ReplaceIf(T const & obj, m2::RectD const & rect, TCompare comp)
    {
      region_t rgn;
      for (size_t i = 0; i < 4; ++i)
      {
        if (i % 2 == 0)
        {
          rgn._M_low_bounds[i] = rect.minX();
          rgn._M_high_bounds[i] = rect.maxX();
        }
        else
        {
          rgn._M_low_bounds[i] = rect.minY();
          rgn._M_high_bounds[i] = rect.maxY();
        }
      }

      store_vec_t isect;

      m_tree.visit_within_range(rgn, insert_if_intersect(isect, rect));

      for (size_t i = 0; i < isect.size(); ++i)
        if (!comp(obj, isect[i]->m_val))
          return;

      for (typename store_vec_t::const_iterator i = isect.begin(); i != isect.end(); ++i)
        m_tree.erase(**i);

      m_tree.insert(value_t(obj, rect));
    }

    template <class TCompare>
    void ReplaceIf(T const & obj, TCompare comp)
    {
      ReplaceIf(obj, obj.GetLimitRect(), comp);
    }

    template <class ToDo>
    void ForEach(ToDo toDo) const
    {
      for (typename tree_t::const_iterator i = m_tree.begin(); i != m_tree.end(); ++i)
        toDo((*i).m_val);
    }

    void Clear() { m_tree.clear(); }
  };
}

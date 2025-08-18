#pragma once

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"

#include "base/logging.hpp"
#include "base/stl_helpers.hpp"

#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "3party/kdtree++/kdtree.hpp"

namespace m4
{
template <typename T>
struct TraitsDef
{
  m2::RectD const LimitRect(T const & t) const { return t.GetLimitRect(); }
};

template <typename T, typename Traits = TraitsDef<T>>
class Tree
{
  class Value
  {
  public:
    using value_type = double;

    template <typename U>
    Value(U && u, m2::RectD const & r) : m_val(std::forward<U>(u))
    {
      SetRect(r);
    }

    bool IsIntersect(m2::RectD const & r) const
    {
      return !((m_pts[2] <= r.minX()) || (m_pts[0] >= r.maxX()) || (m_pts[3] <= r.minY()) || (m_pts[1] >= r.maxY()));
    }

    bool operator==(Value const & r) const { return (m_val == r.m_val); }

    std::string DebugPrint() const
    {
      std::ostringstream out;

      out << DebugPrint(m_val) << ", (" << m_pts[0] << ", " << m_pts[1] << ", " << m_pts[2] << ", " << m_pts[3] << ")";

      return out.str();
    }

    double operator[](size_t i) const { return m_pts[i]; }

    m2::RectD GetRect() const { return m2::RectD(m_pts[0], m_pts[1], m_pts[2], m_pts[3]); }

    T m_val;
    double m_pts[4];

  private:
    void SetRect(m2::RectD const & r)
    {
      m_pts[0] = r.minX();
      m_pts[1] = r.minY();
      m_pts[2] = r.maxX();
      m_pts[3] = r.maxY();
    }
  };

  KDTree::KDTree<4, Value> m_tree;

  // Do-class for rect-iteration in tree.
  template <typename ToDo>
  class for_each_helper
  {
  public:
    for_each_helper(m2::RectD const & r, ToDo && toDo) : m_rect(r), m_toDo(std::forward<ToDo>(toDo)) {}

    bool ScanLeft(size_t plane, Value const & v) const
    {
      switch (plane & 3)  // % 4
      {
      case 2: return m_rect.minX() < v[2];
      case 3: return m_rect.minY() < v[3];
      default: return true;
      }
    }

    bool ScanRight(size_t plane, Value const & v) const
    {
      switch (plane & 3)  // % 4
      {
      case 0: return m_rect.maxX() > v[0];
      case 1: return m_rect.maxY() > v[1];
      default: return true;
      }
    }

    void operator()(Value const & v) const
    {
      if (v.IsIntersect(m_rect))
        m_toDo(v);
    }

    bool DoIfIntersects(Value const & v) const
    {
      if (v.IsIntersect(m_rect))
        return m_toDo(v);
      return false;
    }

  private:
    m2::RectD const & m_rect;
    ToDo m_toDo;
  };

  template <typename ToDo>
  for_each_helper<ToDo> GetFunctor(m2::RectD const & rect, ToDo && toDo) const
  {
    return for_each_helper<ToDo>(rect, std::forward<ToDo>(toDo));
  }

protected:
  Traits m_traits;
  m2::RectD GetLimitRect(T const & t) const { return m_traits.LimitRect(t); }

public:
  Tree(Traits const & traits = Traits()) : m_traits(traits) {}

  using elem_t = T;

  template <typename U>
  void Add(U && obj)
  {
    Add(std::forward<U>(obj), GetLimitRect(obj));
  }

  template <typename U>
  void Add(U && obj, m2::RectD const & rect)
  {
    m_tree.insert(Value(std::forward<U>(obj), rect));
  }

private:
  template <typename Compare>
  void ReplaceImpl(T const & obj, m2::RectD const & rect, Compare comp)
  {
    bool skip = false;
    std::vector<Value const *> isect;

    m_tree.for_each(GetFunctor(rect, [&](Value const & v)
    {
      if (skip)
        return;

      switch (comp(obj, v.m_val))
      {
      case 1: isect.push_back(&v); break;
      case -1: skip = true; break;
      }
    }));

    if (skip)
      return;

    for (Value const * v : isect)
      m_tree.erase(*v);

    Add(obj, rect);
  }

public:
  template <typename Compare>
  void ReplaceAllInRect(T const & obj, Compare comp)
  {
    ReplaceImpl(obj, GetLimitRect(obj), [&comp](T const & t1, T const & t2) { return comp(t1, t2) ? 1 : -1; });
  }

  template <typename Equal, typename Compare>
  void ReplaceEqualInRect(T const & obj, Equal eq, Compare comp)
  {
    ReplaceImpl(obj, GetLimitRect(obj), [&](T const & t1, T const & t2)
    {
      if (eq(t1, t2))
        return comp(t1, t2) ? 1 : -1;
      else
        return 0;
    });
  }

  void Erase(T const & obj, m2::RectD const & r)
  {
    Value val(obj, r);
    m_tree.erase_exact(val);
  }

  void Erase(T const & obj)
  {
    Value val(obj, m_traits.LimitRect(obj));
    m_tree.erase_exact(val);
  }

  template <typename ToDo>
  void ForEach(ToDo && toDo) const
  {
    for (Value const & v : m_tree)
      toDo(v.m_val);
  }

  template <typename ToDo>
  bool ForAny(ToDo && toDo) const
  {
    for (Value const & v : m_tree)
      if (toDo(v.m_val))
        return true;

    return false;
  }

  template <typename ToDo>
  void ForEachEx(ToDo && toDo) const
  {
    for (Value const & v : m_tree)
      toDo(v.GetRect(), v.m_val);
  }

  template <typename ToDo>
  bool FindNode(ToDo && toDo) const
  {
    for (Value const & v : m_tree)
      if (toDo(v.m_val))
        return true;

    return false;
  }

  template <typename ToDo>
  bool ForAnyInRect(m2::RectD const & rect, ToDo && toDo) const
  {
    return m_tree.for_any(GetFunctor(rect, [&toDo](Value const & v) { return toDo(v.m_val); }));
  }

  template <typename ToDo>
  void ForEachInRect(m2::RectD const & rect, ToDo && toDo) const
  {
    m_tree.for_each(GetFunctor(rect, [&toDo](Value const & v) { toDo(v.m_val); }));
  }

  template <typename ToDo>
  void ForEachInRectEx(m2::RectD const & rect, ToDo && toDo) const
  {
    m_tree.for_each(GetFunctor(rect, [&toDo](Value const & v) { toDo(v.GetRect(), v.m_val); }));
  }

  bool IsEmpty() const { return m_tree.empty(); }

  size_t GetSize() const { return m_tree.size(); }

  void Clear() { m_tree.clear(); }

  std::string DebugPrint() const
  {
    std::ostringstream out;
    for (Value const & v : m_tree.begin())
      out << v.DebugPrint() << ", ";
    return out.str();
  }
};

template <typename T, typename Traits>
std::string DebugPrint(Tree<T, Traits> const & t)
{
  return t.DebugPrint();
}
}  // namespace m4

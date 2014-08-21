#pragma once

#include "../std/vector.hpp"
#include "../std/shared_ptr.hpp"

#include "point2d.hpp"

namespace m2
{

class Spline
{
public:
  class iterator
  {
  public:
    PointF m_pos;
    PointF m_dir;
    PointF m_avrDir;
    iterator();
    void Attach(Spline const & spl);
    void Step(float speed);
    bool BeginAgain() const;

  private:
    friend class Spline;
    float GetDistance() const;
    int GetIndex() const;

  private:
    bool m_checker;
    Spline const * m_spl;
    int m_index;
    float m_dist;
  };

public:
  Spline() {}
  Spline(vector<PointF> const & path);
  Spline const & operator = (Spline const & spl);

  void AddPoint(PointF const & pt);
  vector<PointF> const & GetPath() const { return m_position; }

  template <typename TFunctor>
  void ForEachNode(iterator const & begin, iterator const & end, TFunctor & f) const
  {
    ASSERT(begin.BeginAgain() == false, ());
    ASSERT(end.BeginAgain() == false, ());

    f(begin.m_pos);

    for (int i = begin.GetIndex() + 1; i <= end.GetIndex(); ++i)
      f(m_position[i]);

    f(end.m_pos);
  }

  bool IsEmpty() const;
  bool IsValid() const;

  float GetLength() const;

private:
  vector<PointF> m_position;
  vector<PointF> m_direction;
  vector<float> m_length;
};

class SharedSpline
{
public:
  SharedSpline() {}
  SharedSpline(vector<PointF> const & path);
  SharedSpline(SharedSpline const & other);
  SharedSpline const & operator= (SharedSpline const & spl);

  bool IsNull() const;
  void Reset(Spline * spline);
  void Reset(vector<PointF> const & path);

  Spline::iterator CreateIterator() const;

  Spline * operator->();
  Spline const * operator->() const;

private:
  shared_ptr<Spline> m_spline;
};

}

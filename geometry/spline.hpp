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
    void Attach(Spline const & S);
    void Step(float speed);
    bool BeginAgain();
  private:
    bool m_checker;
    Spline const * m_spl;
    int m_index;
    float m_dist;
  };

public:
  Spline() : m_lengthAll(0.0f) {}
  Spline(vector<PointF> const & path) { FromArray(path); }
  void FromArray(vector<PointF> const & path);
  void AddPoint(PointF const & pt);
  Spline const & operator = (Spline const & spl);
  float GetLength() const { return m_lengthAll; }

private:
  float m_lengthAll;
  vector<PointF> m_position;
  vector<PointF> m_direction;
  vector<float> m_length;
};

class SharedSpline
{
public:
  SharedSpline(vector<PointF> const & path);
  SharedSpline(SharedSpline const & other);

  SharedSpline const & operator= (SharedSpline const & spl);
  float GetLength() const;
  Spline::iterator CreateIterator();

private:
  shared_ptr<Spline> m_spline;
};

}

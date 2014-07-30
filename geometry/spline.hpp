#pragma once

#include "../std/vector.hpp"

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
    iterator()
      : m_pos(PointF()), m_dir(PointF()), m_avrDir(PointF()),
      m_checker(false), m_spl(NULL), m_index(0), m_dist(0) {}
    void Attach(Spline const & S);
    void Step(float speed);
    bool beginAgain();
  private:
    bool m_checker;
    Spline const * m_spl;
    int m_index;
    float m_dist;
  };

public:
  Spline() : m_lengthAll(0.1f) {}
  void FromArray(vector<PointF> const & path);
  Spline const & operator = (Spline const & spl);
  float getLength() const { return m_lengthAll; }

private:
  float m_lengthAll;
  vector<PointF> m_position;
  vector<PointF> m_direction;
  vector<float> m_length;
};

}

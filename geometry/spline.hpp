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
      : m_spl(NULL), m_index(0), m_dist(0), m_pos(PointF()),
      m_dir(PointF()), m_avrDir(PointF()), m_checker(false) {}
    void Attach(Spline const & S);
    void Step(float speed);   // Speed MUST BE > 0.0f !!!
    bool beginAgain();
  private:
    bool m_checker;
    Spline const * m_spl;
    int m_index;
    float m_dist;
  };

public:
  Spline(){}
  void FromArray(vector<PointF> const & path);
  Spline const & operator = (Spline const & spl);

private:
  float m_lengthAll;
  vector<PointF> m_position;
  vector<PointF> m_direction;
  vector<float> m_length;
};

}

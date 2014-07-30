#include "spline.hpp"

namespace m2
{

void Spline::FromArray(vector<PointF> const & path)
{
  m_position.assign(path.begin(), path.end() - 1);
  int cnt = m_position.size();
  m_direction = vector<PointF>(cnt);
  m_length = vector<float>(cnt);

  for(int i = 0; i < cnt; ++i)
  {
    m_direction[i] = path[i+1] - path[i];
    m_length[i] = m_direction[i].Length();
    m_direction[i] = m_direction[i].Normalize();
    m_lengthAll += m_length[i];
  }
}

Spline const & Spline::operator = (Spline const & spl)
{
  if(&spl != this)
  {
    m_lengthAll = spl.m_lengthAll;
    m_position = spl.m_position;
    m_direction = spl.m_direction;
    m_length = spl.m_length;
  }
  return *this;
}

Spline::iterator::iterator()
  : m_checker(false)
  , m_spl(NULL)
  , m_index(0)
  , m_dist(0) {}

void Spline::iterator::Attach(Spline const & S)
{
  m_spl = &S;
  m_index = 0;
  m_dist = 0;
  m_checker = false;
  m_dir = m_spl->m_direction[m_index];
  m_avrDir = m_spl->m_direction[m_index];
  m_pos = m_spl->m_position[m_index] + m_dir * m_dist;
}

void Spline::iterator::Step(float speed)
{
  m_dist += speed;
  while(m_dist > m_spl->m_length[m_index])
  {
    m_dist -= m_spl->m_length[m_index];
    m_index++;
    if(m_index >= m_spl->m_position.size())
    {
      m_index = 0;
      m_checker = true;
    }
  }
  m_dir = m_spl->m_direction[m_index];
  m_avrDir = -m_pos;
  m_pos = m_spl->m_position[m_index] + m_dir * m_dist;
  m_avrDir += m_pos;
}

bool Spline::iterator::BeginAgain()
{
  return m_checker;
}

}


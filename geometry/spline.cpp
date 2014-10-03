#include "spline.hpp"

#include "../base/logging.hpp"

#include "../std/numeric.hpp"

namespace m2
{

Spline::Spline(vector<PointD> const & path)
{
  ASSERT(path.size() > 1, ("Wrong path size!"));
  m_position.assign(path.begin(), path.end());
  int cnt = m_position.size() - 1;
  m_direction = vector<PointD>(cnt);
  m_length = vector<double>(cnt);

  for(int i = 0; i < cnt; ++i)
  {
    m_direction[i] = path[i+1] - path[i];
    m_length[i] = m_direction[i].Length();
    m_direction[i] = m_direction[i].Normalize();
  }
}

void Spline::AddPoint(PointD const & pt)
{
  /// TODO remove this check when fix generator.
  /// Now we have line objects with zero length segments
  if (!IsEmpty() && (pt - m_position.back()).IsAlmostZero())
  {
    LOG(LDEBUG, ("Found seqment with zero lenth (the ended points are same)"));
    return;
  }

  if(IsEmpty())
    m_position.push_back(pt);
  else
  {
    PointD dir = pt - m_position.back();
    m_position.push_back(pt);
    m_length.push_back(dir.Length());
    m_direction.push_back(dir.Normalize());
  }
}

bool Spline::IsEmpty() const
{
  return m_position.empty();
}

bool Spline::IsValid() const
{
  return m_position.size() > 1;
}

Spline const & Spline::operator = (Spline const & spl)
{
  if(&spl != this)
  {
    m_position = spl.m_position;
    m_direction = spl.m_direction;
    m_length = spl.m_length;
  }
  return *this;
}

double Spline::GetLength() const
{
  return accumulate(m_length.begin(), m_length.end(), 0.0);
}

Spline::iterator::iterator()
  : m_checker(false)
  , m_spl(NULL)
  , m_index(0)
  , m_dist(0) {}

void Spline::iterator::Attach(Spline const & spl)
{
  m_spl = &spl;
  m_index = 0;
  m_dist = 0;
  m_checker = false;
  m_dir = m_spl->m_direction[m_index];
  m_avrDir = m_spl->m_direction[m_index];
  m_pos = m_spl->m_position[m_index] + m_dir * m_dist;
}

void Spline::iterator::Step(double speed)
{
  m_dist += speed;
  if (m_checker)
  {
    m_pos = m_spl->m_position[m_index] + m_dir * m_dist;
    return;
  }
  while (m_dist > m_spl->m_length[m_index])
  {
    m_dist -= m_spl->m_length[m_index];
    m_index++;
    if (m_index >= m_spl->m_direction.size())
    {
      m_index--;
      m_dist += m_spl->m_length[m_index];
      m_checker = true;
      break;
    }
  }
  m_dir = m_spl->m_direction[m_index];
  m_avrDir = -m_pos;
  m_pos = m_spl->m_position[m_index] + m_dir * m_dist;
  m_avrDir += m_pos;
}

void Spline::iterator::StepBack(double speed)
{
  m_dist -= speed;
  while(m_dist < 0.0f)
  {
    m_index--;
    m_dist += m_spl->m_length[m_index];
  }
  m_dir = m_spl->m_direction[m_index];
  m_avrDir = -m_pos;
  m_pos = m_spl->m_position[m_index] + m_dir * m_dist;
  m_avrDir += m_pos;
}

double Spline::iterator::GetLength() const
{
  return accumulate(m_spl->m_length.begin(), m_spl->m_length.begin() + m_index, m_dist);
}

double Spline::iterator::GetFullLength() const
{
  return m_spl->GetLength();
}

bool Spline::iterator::BeginAgain() const
{
  return m_checker;
}

double Spline::iterator::GetDistance() const
{
  return m_dist;
}

int Spline::iterator::GetIndex() const
{
  return m_index;
}

SharedSpline::SharedSpline(vector<PointD> const & path)
{
  m_spline.reset(new Spline(path));
}

SharedSpline::SharedSpline(SharedSpline const & other)
{
  if (this != &other)
    m_spline = other.m_spline;
}

SharedSpline const & SharedSpline::operator= (SharedSpline const & spl)
{
  if (this != &spl)
    m_spline = spl.m_spline;
  return *this;
}

bool SharedSpline::IsNull() const
{
  return m_spline == NULL;
}

void SharedSpline::Reset(Spline * spline)
{
  m_spline.reset(spline);
}

void SharedSpline::Reset(vector<PointD> const & path)
{
  m_spline.reset(new Spline(path));
}

Spline::iterator SharedSpline::CreateIterator() const
{
  Spline::iterator result;
  result.Attach(*m_spline.get());
  return result;
}

Spline * SharedSpline::operator->()
{
  ASSERT(!IsNull(), ());
  return m_spline.get();
}

Spline const * SharedSpline::operator->() const
{
  ASSERT(!IsNull(), ());
  return m_spline.get();
}

}


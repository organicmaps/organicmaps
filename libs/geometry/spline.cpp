#include "geometry/spline.hpp"
#include "geometry/line2d.hpp"

#include "base/logging.hpp"

#include <numeric>

namespace m2
{
Spline::Spline(std::vector<PointD> const & path) : m_position(path)
{
  InitDirections();
}

Spline::Spline(std::vector<PointD> && path) : m_position(std::move(path))
{
  InitDirections();
}

Spline::Spline(size_t reservedSize)
{
  Reserve(reservedSize);
}

SplineEx::SplineEx(size_t reservedSize) : Spline(reservedSize) {}

void Spline::Reserve(size_t sz)
{
  ASSERT_GREATER(sz, 0, ());
  m_position.reserve(sz);
  m_direction.reserve(sz - 1);
  m_length.reserve(sz - 1);
}

void Spline::AddPoint(PointD const & pt)
{
  if (!IsEmpty())
  {
    /// @todo remove this check when fix generator.
    /// Now we have line objects with zero length segments
    /// https://jira.mail.ru/browse/MAPSME-3561
    PointD const dir = pt - m_position.back();
    if (dir.IsAlmostZero())
      return;

    double const len = dir.Length();
    ASSERT_GREATER(len, 0, ());
    m_length.push_back(len);
    m_direction.push_back(dir / len);
  }

  m_position.push_back(pt);
}

void Spline::ReplacePoint(PointD const & pt)
{
  ASSERT_GREATER(m_position.size(), 1, ());
  ASSERT(!m_length.empty(), ());
  ASSERT(!m_direction.empty(), ());
  m_position.pop_back();
  m_length.pop_back();
  m_direction.pop_back();
  AddPoint(pt);
}

bool Spline::IsProlonging(PointD const & pt) const
{
  size_t const sz = m_position.size();
  if (sz < 2)
    return false;

  PointD dir = pt - m_position.back();
  if (dir.IsAlmostZero())
    return true;
  dir = dir.Normalize();

  ASSERT(!m_direction.empty(), ());
  // Some cos(angle) == 1 (angle == 0) threshold.
  return std::fabs(DotProduct(m_direction.back(), dir)) > 0.995;
}

m2::RectD Spline::GetRect() const
{
  m2::RectD r;
  for (auto const & p : m_position)
    r.Add(p);
  return r;
}

size_t Spline::GetSize() const
{
  return m_position.size();
}

void Spline::Clear()
{
  m_position.clear();
  m_direction.clear();
  m_length.clear();
}

bool Spline::IsEmpty() const
{
  return m_position.empty();
}

bool Spline::IsValid() const
{
  return m_position.size() > 1;
}

Spline::iterator Spline::GetPoint(double step) const
{
  iterator it;
  it.Attach(*this);
  it.Advance(step);
  return it;
}

double Spline::GetLength() const
{
  return std::accumulate(m_length.begin(), m_length.end(), 0.0);
}

double Spline::GetLastLength() const
{
  ASSERT(!m_length.empty(), ());
  return m_length.back();
}

std::pair<PointD, double> Spline::GetTangentAndLength(size_t i) const
{
  ASSERT_LESS(i, m_length.size(), ());
  return {m_direction[i], m_length[i]};
}

void Spline::InitDirections()
{
  ASSERT_GREATER(m_position.size(), 1, ());
  size_t const sz = m_position.size() - 1;

  ASSERT(m_direction.empty() && m_length.empty(), ());
  m_direction.resize(sz);
  m_length.resize(sz);

  for (size_t i = 0; i < sz; ++i)
  {
    m_direction[i] = m_position[i + 1] - m_position[i];
    double const len = m_direction[i].Length();
    ASSERT_GREATER(len, 0, (i));
    m_length[i] = len;
    m_direction[i] = m_direction[i] / len;
  }
}

void Spline::Equidistant(double dist, Spline & res) const
{
  size_t sz = m_position.size();
  ASSERT_GREATER(sz, 1, ());
  res.Reserve(sz);

  auto const Norm = [](PointD const & p) { return PointD{p.y, -p.x}; };

  res.m_position.push_back(m_position[0] + Norm(m_direction[0]) * dist);
  res.m_direction.push_back(m_direction[0]);

  sz -= 1;
  for (size_t i = 1; i < sz; ++i)
  {
    Line2D const l1(res.m_position.back(), res.m_direction.back());
    Line2D const l2(m_position[i] + Norm(m_direction[i]) * dist, m_direction[i]);

    /// @todo It doesn't take into account when the equdistant angle becomes degenerated.

    // Geometry meaning of this constant is |sin(A)| between vectors (cross product of unit vectors).
    // 0.01 is less than 1 degree, so no need to intersect.
    double constexpr eps = 0.01;

    auto const r = Intersect(l1, l2, eps);
    if (r.m_type == IntersectionResult::Type::One)
      res.m_position.push_back(r.m_point);
    else
    {
      /// @todo Can make 2 points (trapezius) for _very_ acute angel.
      res.m_position.push_back(l2.m_point);
    }

    res.m_direction.push_back(m_direction[i]);
    res.m_length.push_back(res.m_position[i - 1].Length(res.m_position[i]));
  }

  res.m_position.push_back(m_position.back() + Norm(m_direction.back()) * dist);
  res.m_length.push_back(res.m_position[sz - 1].Length(res.m_position[sz]));
}

std::string DebugPrint(Spline const & s)
{
  return ::DebugPrint(s.m_position);
}

Spline::iterator::iterator() : m_checker(false), m_spl(NULL), m_index(0), m_dist(0) {}

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

bool Spline::iterator::IsAttached() const
{
  return m_spl != nullptr;
}

void Spline::iterator::Advance(double step)
{
  if (step < 0.0)
    AdvanceBackward(step);
  else
    AdvanceForward(step);
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

size_t Spline::iterator::GetIndex() const
{
  return m_index;
}

void Spline::iterator::AdvanceBackward(double step)
{
  m_dist += step;
  while (m_dist < 0.0f)
  {
    if (m_index == 0)
    {
      m_checker = true;
      m_pos = m_spl->m_position[m_index];
      m_dir = m_spl->m_direction[m_index];
      m_avrDir = m2::PointD::Zero();
      m_dist = 0.0;
      return;
    }

    --m_index;

    m_dist += m_spl->m_length[m_index];
  }

  m_dir = m_spl->m_direction[m_index];
  m_avrDir = -m_pos;
  m_pos = m_spl->m_position[m_index] + m_dir * m_dist;
  m_avrDir += m_pos;
}

void Spline::iterator::AdvanceForward(double step)
{
  m_dist += step;
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

SharedSpline::SharedSpline(std::vector<PointD> const & path) : m_spline(std::make_shared<Spline>(path)) {}

SharedSpline::SharedSpline(std::vector<PointD> && path) : m_spline(std::make_shared<Spline>(std::move(path))) {}

bool SharedSpline::IsNull() const
{
  return m_spline == nullptr;
}

void SharedSpline::Reset(Spline * spline)
{
  m_spline.reset(spline);
}

// void SharedSpline::Reset(std::vector<PointD> const & path)
//{
//   m_spline.reset(new Spline(path));
// }

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
  return Get();
}

Spline const * SharedSpline::Get() const
{
  ASSERT(!IsNull(), ());
  return m_spline.get();
}

SharedSpline SharedSpline::Equidistant(double dist) const
{
  if (fabs(dist) < 1.0E-6)  // kMwmPointAccuracy * 0.1
    return *this;

  SharedSpline res;
  res.Reset(new Spline());
  m_spline->Equidistant(dist, *res.m_spline);
  return res;
}

/// @todo Should be friend to the Spline.
bool AlmostEqualAbs(m2::Spline const & s1, m2::Spline const & s2, double eps)
{
  size_t const sz = s1.GetPath().size();
  if (sz != s2.GetPath().size())
    return false;

  for (size_t i = 0; i < sz; ++i)
  {
    if (!AlmostEqualAbs(s1.GetPath()[i], s2.GetPath()[i], eps))
      return false;

    if (i < sz - 1)
    {
      auto const r1 = s1.GetTangentAndLength(i);
      auto const r2 = s2.GetTangentAndLength(i);

      if (!AlmostEqualAbs(r1.first, r2.first, eps) || !::AlmostEqualAbs(r1.second, r2.second, eps))
        return false;
    }
  }

  return true;
}
}  // namespace m2

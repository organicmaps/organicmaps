#pragma once

#include "std/vector.hpp"
#include "std/shared_ptr.hpp"

#include "geometry/point2d.hpp"

namespace m2
{

class Spline
{
public:
  class iterator
  {
  public:
    PointD m_pos;
    PointD m_dir;
    PointD m_avrDir;

    iterator();
    iterator(iterator const & other);
    iterator & operator=(iterator const & other);

    void Attach(Spline const & spl);
    void Advance(double step);
    bool BeginAgain() const;
    double GetLength() const;
    double GetFullLength() const;

    size_t GetIndex() const;
    bool IsAttached() const;

  private:
    friend class Spline;
    double GetDistance() const;

    void AdvanceForward(double step);
    void AdvanceBackward(double step);

  private:
    bool m_checker;
    Spline const * m_spl;
    size_t m_index;
    double m_dist;
  };

public:
  Spline() {}
  Spline(size_t reservedSize);
  Spline(vector<PointD> const & path);
  Spline const & operator = (Spline const & spl);

  void AddPoint(PointD const & pt);
  void ReplacePoint(PointD const & pt);
  bool IsPrelonging(PointD const & pt);
  size_t GetSize() const;
  vector<PointD> const & GetPath() const { return m_position; }
  vector<double> const & GetLengths() const { return m_length; }
  vector<PointD> const & GetDirections() const { return m_direction; }
  void Clear();

  iterator GetPoint(double step) const;

  template <typename TFunctor>
  void ForEachNode(iterator const & begin, iterator const & end, TFunctor const & f) const
  {
    ASSERT(begin.BeginAgain() == false, ());
    ASSERT(end.BeginAgain() == false, ());

    f(begin.m_pos);

    for (size_t i = begin.GetIndex() + 1; i <= end.GetIndex(); ++i)
      f(m_position[i]);

    f(end.m_pos);
  }

  bool IsEmpty() const;
  bool IsValid() const;

  double GetLength() const;

private:
  vector<PointD> m_position;
  vector<PointD> m_direction;
  vector<double> m_length;
};

class SharedSpline
{
public:
  SharedSpline() {}
  SharedSpline(vector<PointD> const & path);
  SharedSpline(SharedSpline const & other);
  SharedSpline const & operator= (SharedSpline const & spl);

  bool IsNull() const;
  void Reset(Spline * spline);
  void Reset(vector<PointD> const & path);

  Spline::iterator CreateIterator() const;

  Spline * operator->();
  Spline const * operator->() const;

  Spline const * Get() const;

private:
  shared_ptr<Spline> m_spline;
};

}

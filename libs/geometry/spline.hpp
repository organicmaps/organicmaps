#pragma once

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"

#include <memory>
#include <vector>

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

    void Attach(Spline const & spl);
    void Advance(double step);
    bool BeginAgain() const;
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
  Spline() = default;
  explicit Spline(size_t reservedSize);
  explicit Spline(std::vector<PointD> path);

  void AddPoint(PointD const & pt);
  /// Variant for callers that already know the direction and length of the
  /// new segment (e.g. clip-from-source-spline). The caller guarantees:
  ///   - the spline is non-empty;
  ///   - |dir| is the unit vector from the current m_position.back() to pt;
  ///   - |len| is the Euclidean distance from m_position.back() to pt
  ///     and is strictly positive (no degenerate zero-length segments).
  /// Skips the sqrt + normalize that the single-arg overload pays.
  void AddPoint(PointD const & pt, PointD const & dir, double len);
  void ReplacePoint(PointD const & pt);
  bool IsProlonging(PointD const & pt) const;

  m2::RectD GetRect() const;
  size_t GetSize() const;
  std::vector<PointD> const & GetPath() const { return m_position; }
  void Clear();

  iterator GetPoint(double step) const;

  bool IsEmpty() const;
  bool IsValid() const;

  double GetLength() const;
  /// @return for (i) -> (i + 1) section.
  std::pair<PointD, double> GetTangentAndLength(size_t i) const;

  void Equidistant(double dist, Spline & res) const;

  friend std::string DebugPrint(Spline const & s);

protected:
  void InitDirections();
  void Reserve(size_t sz);

  std::vector<PointD> m_position;
  std::vector<PointD> m_direction;
  std::vector<double> m_length;
};

class SplineEx : public Spline
{
public:
  explicit SplineEx(size_t reservedSize = 2) : Spline(reservedSize) {}

  std::vector<double> const & GetLengths() const { return m_length; }
  std::vector<PointD> const & GetDirections() const { return m_direction; }
};

class SharedSpline
{
public:
  SharedSpline() = default;
  explicit SharedSpline(std::vector<PointD> path) : m_spline(std::make_shared<Spline>(std::move(path))) {}
  SharedSpline(std::unique_ptr<Spline> p) : m_spline(std::move(p)) {}

  bool IsNull() const { return m_spline == nullptr; }

  Spline::iterator CreateIterator() const;

  Spline const & operator*() const { return *Get(); }
  Spline const * operator->() const { return Get(); }
  Spline const * Get() const
  {
    ASSERT(!IsNull(), ());
    return m_spline.get();
  }

  SharedSpline Equidistant(double dist) const;

private:
  std::shared_ptr<Spline> m_spline;
};

bool AlmostEqualAbs(Spline const & s1, Spline const & s2, double eps);
}  // namespace m2

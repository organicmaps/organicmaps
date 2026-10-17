#pragma once

#include "map/elevation_info.hpp"

#include "base/buffer_vector.hpp"

#include <array>
#include <functional>
#include <limits>
#include <optional>
#include <span>

struct TrackStatistics;

// Shared GPS elevation policy. Statistics retain no history beyond a bounded mutable tail.
class TrackElevation
{
public:
  using Point = ElevationInfo::Point;
  using Altitude = geometry::Altitude;
  static size_t constexpr kWindowIntervals = 256;
  static size_t constexpr kOutlierLimit = 16;
  static size_t constexpr kMaxTailPoints = kWindowIntervals + 1 + kOutlierLimit + 1;
  static double constexpr kDeviation = 1.415;

  struct Totals
  {
    uint32_t m_ascent = 0;
    uint32_t m_descent = 0;
    uint32_t m_smallAscent = 0;
    uint32_t m_smallDescent = 0;
    Altitude m_min = std::numeric_limits<Altitude>::max();
    Altitude m_max = std::numeric_limits<Altitude>::min();

    void Add(Totals const & other);
    void Apply(TrackStatistics & statistics) const;
  };

  // Receives only newly finalized vertices, excluding an already emitted shared anchor.
  using Output = std::function<void(std::span<Point const>)>;
  void Add(Point point, Output const & output = {});
  Totals const & GetTotals() const;
  // Includes the shared anchor. Queries never finalize the current endpoint.
  std::span<Point const> GetTail() const;
  bool HasPrefix() const { return m_prefix.m_initialized; }

  static Totals Process(std::vector<ElevationInfo::GeometryLine> const & lines,
                        std::vector<ElevationInfo::Points> * profile = nullptr,
                        std::function<bool()> const & isCancelled = {});

private:
  static Altitude constexpr kFlatNoiseRange = 2;
  using Points = buffer_vector<Point, kMaxTailPoints>;

  struct SmallVariation
  {
    void Add(Altitude altitude);
    uint32_t GetAscent() const;
    uint32_t GetDescent() const;

    int m_direction = 0;
    int m_low = 0;
    int m_high = 0;
    int m_anchor = 0;
    int m_extreme = 0;
    uint32_t m_ascent = 0;
    uint32_t m_descent = 0;
    bool m_initialized = false;
  };

  struct Accumulator
  {
    void Add(Altitude altitude);
    Totals GetTotals() const;
    Totals m_totals;
    SmallVariation m_small;
    Altitude m_reference = 0;
    bool m_initialized = false;
  };

  static bool IsOutlier(Point a, Point b, Point c);
  static void Constrain(Point anchor, Point point, double & lower, double & upper);
  static void Simplify(std::span<Point const> points, Points & output);
  Point Interpolate(Point point, Point right) const;
  void Confirm(Point point, Output const & output);
  void Update() const;

  std::array<Point, kWindowIntervals + 1> m_tail{};
  size_t m_size = 0;
  Altitude m_minTail = std::numeric_limits<Altitude>::max();
  Altitude m_maxTail = std::numeric_limits<Altitude>::min();
  double m_lowerSlope = -std::numeric_limits<double>::max();
  double m_upperSlope = std::numeric_limits<double>::max();
  Accumulator m_prefix;

  std::array<Point, kOutlierLimit> m_outliers{};
  size_t m_outlierCount = 0;
  bool m_longOutliers = false;
  Point m_lastGood{};
  std::optional<Point> m_before;
  std::optional<Point> m_previous;

  mutable Points m_preview;
  mutable Totals m_totals;
  mutable bool m_dirty = true;
};

#pragma once
#include "result.hpp"
#include "../indexer/feature.hpp"

namespace search
{
namespace impl
{

class IntermediateResult
{
public:
  enum ResultType
  {
    RESULT_LATLON,
    RESULT_CATEGORY,
    RESULT_FEATURE
  };

  // For RESULT_FEATURE.
  IntermediateResult(m2::RectD const & viewportRect,
                     FeatureType const & f,
                     string const & displayName, string const & regionName);

  // For RESULT_LATLON.
  IntermediateResult(m2::RectD const & viewportRect, string const & regionName,
                     double lat, double lon, double precision);

  // For RESULT_CATEGORY.
  IntermediateResult(string const & name, int penalty);

  Result GenerateFinalResult() const;

  /// Results order functor.
  /*
  struct LessOrderF
  {
    bool operator() (IntermediateResult const & r1, IntermediateResult const & r2) const;
  };
  */

  static bool LessRank(IntermediateResult const & r1, IntermediateResult const & r2);
  static bool LessDistance(IntermediateResult const & r1, IntermediateResult const & r2);
  static bool LessViewportDistance(IntermediateResult const & r1, IntermediateResult const & r2);

  /// Filter equal features for different mwm's.
  class StrictEqualF
  {
    IntermediateResult const & m_r;
  public:
    StrictEqualF(IntermediateResult const & r) : m_r(r) {}
    bool operator() (IntermediateResult const & r) const;
  };

  /// To filter equal linear objects.
  //@{
  struct LessLinearTypesF
  {
    bool operator() (IntermediateResult const & r1, IntermediateResult const & r2) const;
  };
  class EqualLinearTypesF
  {
  public:
    bool operator() (IntermediateResult const & r1, IntermediateResult const & r2) const;
  };
  //@}

  string DebugPrint() const;

private:
  static double ResultDistance(m2::PointD const & viewportCenter,
                               m2::PointD const & featureCenter);
  static double ResultDirection(m2::PointD const & viewportCenter,
                                m2::PointD const & featureCenter);
  static int ViewportDistance(m2::RectD const & viewport, m2::PointD const & p);

  string m_str, m_completionString, m_region;

  m2::RectD m_rect;
  uint32_t m_type;

  double m_distance;
  double m_direction;
  int m_viewportDistance;

  ResultType m_resultType;
  uint8_t m_searchRank;
};

inline string DebugPrint(IntermediateResult const & t)
{
  return t.DebugPrint();
}

}  // namespace search::impl
}  // namespace search

namespace boost
{
  inline string DebugPrint(shared_ptr<search::impl::IntermediateResult> const & p)
  {
    return DebugPrint(*p);
  }
}

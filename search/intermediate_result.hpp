#pragma once
#include "result.hpp"

#include "../std/shared_ptr.hpp"


class FeatureType;

namespace storage { class CountryInfoGetter; }

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
  IntermediateResult(m2::RectD const & viewportRect, m2::PointD const & pos,
                     FeatureType const & f,
                     string const & displayName,
                     string const & fileName);

  // For RESULT_LATLON.
  IntermediateResult(m2::RectD const & viewportRect, m2::PointD const & pos,
                     double lat, double lon, double precision);

  // For RESULT_CATEGORY.
  IntermediateResult(string const & name, int penalty);

  Result GenerateFinalResult(storage::CountryInfoGetter const * pInfo) const;

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

  string m_str, m_completionString;

  class RegionInfo
  {
    string m_file;
    m2::PointD m_point;
    bool m_valid;

  public:
    RegionInfo() : m_valid(false) {}

    void SetName(string const & s) { m_file = s; }
    void SetPoint(m2::PointD const & p)
    {
      m_point = p;
      m_valid = true;
    }

    string GetRegion(storage::CountryInfoGetter const * pInfo) const;
  } m_region;

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

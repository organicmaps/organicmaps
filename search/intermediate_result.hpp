#pragma once
#include "result.hpp"

#include "../indexer/feature_data.hpp"

#include "../base/string_utils.hpp"

#include "../std/shared_ptr.hpp"
#include "../std/map.hpp"


class FeatureType;

namespace storage
{
  class CountryInfoGetter;
  struct CountryInfo;
}

namespace search
{
namespace impl
{

/// First results class. Objects are creating during search in trie.
class PreResult1
{
  friend class PreResult2;

  m2::PointD m_center;
  double m_distance;
  size_t m_mwmID;
  uint32_t m_featureID;
  uint8_t m_viewportDistance;
  uint8_t m_rank;

  void CalcParams(m2::RectD const & viewportRect, m2::PointD const & pos);

public:
  PreResult1(uint32_t fID, uint8_t rank, m2::PointD const & center, size_t mwmID,
             m2::PointD const & pos, m2::RectD const & viewport);

  static bool LessRank(PreResult1 const & r1, PreResult1 const & r2);
  static bool LessDistance(PreResult1 const & r1, PreResult1 const & r2);
  static bool LessViewportDistance(PreResult1 const & r1, PreResult1 const & r2);

  inline pair<uint32_t, size_t> GetID() const { return make_pair(m_featureID, m_mwmID); }
};


/// Second result class. Objects are creating during reading of features.
class PreResult2
{
public:
  enum ResultType
  {
    RESULT_LATLON,
    RESULT_CATEGORY,
    RESULT_FEATURE
  };

  // For RESULT_FEATURE.
  PreResult2(FeatureType const & f, PreResult1 const & res,
             string const & displayName, string const & fileName);

  // For RESULT_LATLON.
  PreResult2(m2::RectD const & viewport, m2::PointD const & pos,
             double lat, double lon);

  // For RESULT_CATEGORY.
  PreResult2(string const & name, int penalty);

  typedef multimap<strings::UniString, uint32_t> CategoriesT;
  Result GenerateFinalResult(storage::CountryInfoGetter const * pInfo,
                             CategoriesT const * pCat) const;

  static bool LessRank(PreResult2 const & r1, PreResult2 const & r2);
  static bool LessDistance(PreResult2 const & r1, PreResult2 const & r2);
  static bool LessViewportDistance(PreResult2 const & r1, PreResult2 const & r2);

  /// Filter equal features for different mwm's.
  class StrictEqualF
  {
    PreResult2 const & m_r;
  public:
    StrictEqualF(PreResult2 const & r) : m_r(r) {}
    bool operator() (PreResult2 const & r) const;
  };

  /// To filter equal linear objects.
  //@{
  struct LessLinearTypesF
  {
    bool operator() (PreResult2 const & r1, PreResult2 const & r2) const;
  };
  class EqualLinearTypesF
  {
  public:
    bool operator() (PreResult2 const & r1, PreResult2 const & r2) const;
  };
  //@}

  string DebugPrint() const;

private:
  string GetFeatureType(CategoriesT const * pCat) const;

  feature::TypesHolder m_types;
  inline uint32_t GetBestType() const
  {
    /// @todo Need to process all types.
    return m_types.GetBestType();
  }

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

    void GetRegion(storage::CountryInfoGetter const * pInfo, storage::CountryInfo & info) const;
  } m_region;

  m2::PointD m_center;
  double m_distance;
  ResultType m_resultType;
  uint8_t m_searchRank;
  uint8_t m_viewportDistance;
};

inline string DebugPrint(PreResult2 const & t)
{
  return t.DebugPrint();
}

}  // namespace search::impl
}  // namespace search

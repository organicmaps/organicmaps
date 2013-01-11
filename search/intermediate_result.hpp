#pragma once
#include "result.hpp"

#include "../indexer/feature_data.hpp"

#include "../base/string_utils.hpp"

#include "../std/shared_ptr.hpp"
#include "../std/map.hpp"


class FeatureType;
class CategoriesHolder;

namespace storage
{
  class CountryInfoGetter;
  struct CountryInfo;
}

namespace search
{
namespace impl
{

template <class T> bool LessRankT(T const & r1, T const & r2);
template <class T> bool LessViewportDistanceT(T const & r1, T const & r2);
template <class T> bool LessDistanceT(T const & r1, T const & r2);

/// First results class. Objects are creating during search in trie.
class PreResult1
{
  friend class PreResult2;
  template <class T> friend bool LessRankT(T const & r1, T const & r2);
  template <class T> friend bool LessViewportDistanceT(T const & r1, T const & r2);
  template <class T> friend bool LessDistanceT(T const & r1, T const & r2);

  m2::PointD m_center;
  double m_distance, m_distanceFromViewportCenter;
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

  inline pair<size_t, uint32_t> GetID() const { return make_pair(m_mwmID, m_featureID); }
  uint8_t GetRank() const { return m_rank; }
};


/// Second result class. Objects are creating during reading of features.
class PreResult2
{
  void CalcParams(m2::RectD const & viewport, m2::PointD const & pos);

public:
  enum ResultType
  {
    RESULT_LATLON,
    RESULT_CATEGORY,
    RESULT_FEATURE
  };

  // For RESULT_FEATURE.
  PreResult2(FeatureType const & f, uint8_t rank,
             m2::RectD const & viewport, m2::PointD const & pos,
             string const & displayName, string const & fileName);

  // For RESULT_LATLON.
  PreResult2(m2::RectD const & viewport, m2::PointD const & pos,
             double lat, double lon);

  // For RESULT_CATEGORY.
  PreResult2(string const & name, int penalty);

  /// @param[in]  pInfo   Need to get region for result.
  /// @param[in]  pCat    Categories need to display readable type string.
  /// @param[in]  pTypes  Set of preffered types that match input tokens by categories.
  /// @param[in]  lang    Current system language.
  Result GenerateFinalResult(storage::CountryInfoGetter const * pInfo,
                             CategoriesHolder const * pCat,
                             set<uint32_t> const * pTypes,
                             int8_t lang) const;

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
  template <class T> friend bool LessRankT(T const & r1, T const & r2);
  template <class T> friend bool LessViewportDistanceT(T const & r1, T const & r2);
  template <class T> friend bool LessDistanceT(T const & r1, T const & r2);

  string GetFeatureType(CategoriesHolder const * pCat,
                        set<uint32_t> const * pTypes,
                        int8_t lang) const;
  m2::RectD GetFinalViewport() const;

  feature::TypesHolder m_types;
  uint32_t GetBestType(set<uint32_t> const * pPrefferedTypes = 0) const;

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

  m2::RectD m_featureRect;
  m2::PointD m_center;
  double m_distance, m_distanceFromViewportCenter;
  ResultType m_resultType;
  uint8_t m_rank;
  uint8_t m_viewportDistance;
};

inline string DebugPrint(PreResult2 const & t)
{
  return t.DebugPrint();
}

}  // namespace search::impl
}  // namespace search

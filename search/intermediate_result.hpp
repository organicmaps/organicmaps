#pragma once
#include "search/result.hpp"
#include "search/v2/pre_ranking_info.hpp"
#include "search/v2/ranking_info.hpp"
#include "search/v2/ranking_utils.hpp"

#include "indexer/feature_data.hpp"

class FeatureType;
class CategoriesHolder;

namespace storage
{
class CountryInfoGetter;
struct CountryInfo;
}

namespace search
{
class ReverseGeocoder;
namespace impl
{
/// First pass results class. Objects are creating during search in trie.
/// Works fast without feature loading and provide ranking.
class PreResult1
{
  friend class PreResult2;

  FeatureID m_id;
  double m_priority;
  int8_t m_viewportID;

  v2::PreRankingInfo m_info;

public:
  PreResult1();

  explicit PreResult1(double priority);

  PreResult1(FeatureID const & fID, double priority, int8_t viewportID,
             v2::PreRankingInfo const & info);

  static bool LessRank(PreResult1 const & r1, PreResult1 const & r2);
  static bool LessPriority(PreResult1 const & r1, PreResult1 const & r2);

  inline FeatureID GetID() const { return m_id; }
  inline double GetPriority() const { return m_priority; }
  inline uint8_t GetRank() const { return m_info.m_rank; }
  inline int8_t GetViewportID() const { return m_viewportID; }
  inline v2::PreRankingInfo const & GetInfo() const { return m_info; }
};


/// Second result class. Objects are creating during reading of features.
/// Read and fill needed info for ranking and getting final results.
class PreResult2
{
  friend class PreResult2Maker;

public:
  enum ResultType
  {
    RESULT_LATLON,
    RESULT_FEATURE,
    RESULT_BUILDING  //!< Buildings are not filtered out in duplicates filter.
  };

  /// For RESULT_FEATURE and RESULT_BUILDING.
  PreResult2(FeatureType const & f, PreResult1 const * p, m2::PointD const & center,
             m2::PointD const & pivot, string const & displayName, string const & fileName);

  /// For RESULT_LATLON.
  PreResult2(double lat, double lon);

  inline search::v2::RankingInfo const & GetRankingInfo() const { return m_info; }

  template <typename TInfo>
  inline void SetRankingInfo(TInfo && info)
  {
    m_info = forward<TInfo>(info);
  }

  /// @param[in]  infoGetter Need to get region for result.
  /// @param[in]  pCat    Categories need to display readable type string.
  /// @param[in]  pTypes  Set of preffered types that match input tokens by categories.
  /// @param[in]  lang    Current system language.
  Result GenerateFinalResult(storage::CountryInfoGetter const & infoGetter,
                             CategoriesHolder const * pCat, set<uint32_t> const * pTypes,
                             int8_t locale, ReverseGeocoder const & coder) const;

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

  bool IsStreet() const;

  inline FeatureID const & GetID() const { return m_id; }
  inline string const & GetName() const { return m_str; }
  inline feature::TypesHolder const & GetTypes() const { return m_types; }

private:
  bool IsEqualCommon(PreResult2 const & r) const;

  FeatureID m_id;
  feature::TypesHolder m_types;

  uint32_t GetBestType(set<uint32_t> const * pPrefferedTypes = 0) const;

  string m_str;

  struct RegionInfo
  {
    string m_file;
    m2::PointD m_point;

    inline void SetParams(string const & file, m2::PointD const & pt)
    {
      m_file = file;
      m_point = pt;
    }

    void GetRegion(storage::CountryInfoGetter const & infoGetter,
                   storage::CountryInfo & info) const;
  } m_region;

  string GetRegionName(storage::CountryInfoGetter const & infoGetter, uint32_t fType) const;

  m2::PointD GetCenter() const { return m_region.m_point; }

  double m_distance;
  ResultType m_resultType;
  v2::RankingInfo m_info;
  feature::EGeomType m_geomType;

  Result::Metadata m_metadata;
};

inline string DebugPrint(PreResult2 const & t)
{
  return t.DebugPrint();
}

}  // namespace search::impl

void ProcessMetadata(FeatureType const & ft, Result::Metadata & meta);

}  // namespace search

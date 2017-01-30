#pragma once

#include "search/pre_ranking_info.hpp"
#include "search/ranking_info.hpp"
#include "search/ranking_utils.hpp"
#include "search/result.hpp"

#include "indexer/feature_data.hpp"

#include "std/set.hpp"

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

/// First pass results class. Objects are creating during search in trie.
/// Works fast without feature loading and provide ranking.
class PreResult1
{
public:
  PreResult1(FeatureID const & fID, PreRankingInfo const & info);

  static bool LessRank(PreResult1 const & r1, PreResult1 const & r2);
  static bool LessDistance(PreResult1 const & r1, PreResult1 const & r2);

  inline FeatureID GetId() const { return m_id; }
  inline double GetDistance() const { return m_info.m_distanceToPivot; }
  inline uint8_t GetRank() const { return m_info.m_rank; }
  inline PreRankingInfo & GetInfo() { return m_info; }
  inline PreRankingInfo const & GetInfo() const { return m_info; }

private:
  friend class PreResult2;

  FeatureID m_id;
  PreRankingInfo m_info;
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
  PreResult2(FeatureType const & f, m2::PointD const & center, m2::PointD const & pivot,
             string const & displayName, string const & fileName);

  /// For RESULT_LATLON.
  PreResult2(double lat, double lon);

  inline search::RankingInfo const & GetRankingInfo() const { return m_info; }

  template <typename TInfo>
  inline void SetRankingInfo(TInfo && info)
  {
    m_info = forward<TInfo>(info);
  }

  /// @param[in]  infoGetter Need to get region for result.
  /// @param[in]  pCat    Categories need to display readable type string.
  /// @param[in]  pTypes  Set of preffered types that match input tokens by categories.
  /// @param[in]  lang    Current system language.
  /// @param[in]  coder   May be nullptr - no need to calculate address.
  Result GenerateFinalResult(storage::CountryInfoGetter const & infoGetter,
                             CategoriesHolder const * pCat, set<uint32_t> const * pTypes,
                             int8_t locale, ReverseGeocoder const * coder) const;

  /// Filter equal features for different mwm's.
  class StrictEqualF
  {
  public:
    StrictEqualF(PreResult2 const & r, double const epsMeters);

    bool operator()(PreResult2 const & r) const;

  private:
    PreResult2 const & m_r;
    double const m_epsMeters;
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
  inline m2::PointD GetCenter() const { return m_region.m_point; }

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

  double m_distance;
  ResultType m_resultType;
  RankingInfo m_info;
  feature::EGeomType m_geomType;

  Result::Metadata m_metadata;
};

inline string DebugPrint(PreResult2 const & t)
{
  return t.DebugPrint();
}

void ProcessMetadata(FeatureType const & ft, Result::Metadata & meta);

class IndexedValue
{
  /// @todo Do not use shared_ptr for optimization issues.
  /// Need to rewrite std::unique algorithm.
  unique_ptr<PreResult2> m_value;

  double m_rank;
  double m_distanceToPivot;

  friend string DebugPrint(IndexedValue const & value)
  {
    ostringstream os;
    os << "IndexedValue [";
    if (value.m_value)
      os << DebugPrint(*value.m_value);
    os << "]";
    return os.str();
  }

public:
  explicit IndexedValue(unique_ptr<PreResult2> value)
    : m_value(move(value)), m_rank(0.0), m_distanceToPivot(numeric_limits<double>::max())
  {
    if (!m_value)
      return;

    auto const & info = m_value->GetRankingInfo();
    m_rank = info.GetLinearModelRank();
    m_distanceToPivot = info.m_distanceToPivot;
  }

  PreResult2 const & operator*() const { return *m_value; }

  inline double GetRank() const { return m_rank; }

  inline double GetDistanceToPivot() const { return m_distanceToPivot; }
};
}  // namespace search

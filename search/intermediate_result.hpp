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

// First pass results class. Objects are created during search in trie.
// Works fast because it does not load features.
class PreRankerResult
{
public:
  PreRankerResult(FeatureID const & id, PreRankingInfo const & info);

  static bool LessRank(PreRankerResult const & r1, PreRankerResult const & r2);
  static bool LessDistance(PreRankerResult const & r1, PreRankerResult const & r2);

  inline FeatureID GetId() const { return m_id; }
  inline double GetDistance() const { return m_info.m_distanceToPivot; }
  inline uint8_t GetRank() const { return m_info.m_rank; }
  inline PreRankingInfo & GetInfo() { return m_info; }
  inline PreRankingInfo const & GetInfo() const { return m_info; }

private:
  friend class RankerResult;

  FeatureID m_id;
  PreRankingInfo m_info;
};

// Second result class. Objects are created during reading of features.
// Read and fill needed info for ranking and getting final results.
class RankerResult
{
public:
  enum Type
  {
    TYPE_LATLON,
    TYPE_FEATURE,
    TYPE_BUILDING  //!< Buildings are not filtered out in duplicates filter.
  };

  /// For RESULT_FEATURE and RESULT_BUILDING.
  RankerResult(FeatureType const & f, m2::PointD const & center, m2::PointD const & pivot,
               string const & displayName, string const & fileName);

  /// For RESULT_LATLON.
  RankerResult(double lat, double lon);

  bool IsStreet() const;

  search::RankingInfo const & GetRankingInfo() const { return m_info; }

  template <typename TInfo>
  inline void SetRankingInfo(TInfo && info)
  {
    m_info = forward<TInfo>(info);
  }

  FeatureID const & GetID() const { return m_id; }
  string const & GetName() const { return m_str; }
  feature::TypesHolder const & GetTypes() const { return m_types; }
  Type const & GetResultType() const { return m_resultType; }
  m2::PointD GetCenter() const { return m_region.m_point; }
  double GetDistance() const { return m_distance; }
  feature::EGeomType GetGeomType() const { return m_geomType; }
  Result::Metadata GetMetadata() const { return m_metadata; }

  double GetDistanceToPivot() const { return m_info.m_distanceToPivot; }
  double GetLinearModelRank() const { return m_info.GetLinearModelRank(); }

  string GetRegionName(storage::CountryInfoGetter const & infoGetter, uint32_t ftype) const;

  bool IsEqualCommon(RankerResult const & r) const;

  uint32_t GetBestType(set<uint32_t> const * pPrefferedTypes = 0) const;

private:
  friend class RankerResultMaker;

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
  };

  RegionInfo m_region;
  FeatureID m_id;
  feature::TypesHolder m_types;
  string m_str;
  double m_distance;
  Type m_resultType;
  RankingInfo m_info;
  feature::EGeomType m_geomType;
  Result::Metadata m_metadata;
};

void ProcessMetadata(FeatureType const & ft, Result::Metadata & meta);

string DebugPrint(RankerResult const & r);
}  // namespace search

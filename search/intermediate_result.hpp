#pragma once

#include "search/pre_ranking_info.hpp"
#include "search/ranking_info.hpp"
#include "search/ranking_utils.hpp"
#include "search/result.hpp"

#include "storage/index.hpp"

#include "indexer/feature_data.hpp"

#include <cstdint>
#include <string>
#include <vector>

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

  static bool LessRankAndPopularity(PreRankerResult const & r1, PreRankerResult const & r2);
  static bool LessDistance(PreRankerResult const & r1, PreRankerResult const & r2);

  struct CategoriesComparator
  {
    bool operator()(PreRankerResult const & lhs, PreRankerResult const & rhs) const;

    m2::RectD m_viewport;
    bool m_positionIsInsideViewport = false;
    bool m_detailedScale = false;
  };

  FeatureID const & GetId() const { return m_id; }
  double GetDistance() const { return m_info.m_distanceToPivot; }
  uint8_t GetRank() const { return m_info.m_rank; }
  uint8_t GetPopularity() const { return m_info.m_popularity; }
  PreRankingInfo & GetInfo() { return m_info; }
  PreRankingInfo const & GetInfo() const { return m_info; }

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
  RankerResult(FeatureType & f, m2::PointD const & center, m2::PointD const & pivot,
               std::string const & displayName, std::string const & fileName);

  /// For RESULT_LATLON.
  RankerResult(double lat, double lon);

  bool IsStreet() const;

  search::RankingInfo const & GetRankingInfo() const { return m_info; }

  template <typename TInfo>
  inline void SetRankingInfo(TInfo && info)
  {
    m_info = std::forward<TInfo>(info);
  }

  FeatureID const & GetID() const { return m_id; }
  std::string const & GetName() const { return m_str; }
  feature::TypesHolder const & GetTypes() const { return m_types; }
  Type const & GetResultType() const { return m_resultType; }
  m2::PointD GetCenter() const { return m_region.m_point; }
  double GetDistance() const { return m_distance; }
  feature::EGeomType GetGeomType() const { return m_geomType; }
  Result::Metadata GetMetadata() const { return m_metadata; }

  double GetDistanceToPivot() const { return m_info.m_distanceToPivot; }
  double GetLinearModelRank() const { return m_info.GetLinearModelRank(); }

  bool GetCountryId(storage::CountryInfoGetter const & infoGetter, uint32_t ftype,
                    storage::TCountryId & countryId) const;

  bool IsEqualCommon(RankerResult const & r) const;

  uint32_t GetBestType(std::vector<uint32_t> const & preferredTypes = {}) const;

private:
  friend class RankerResultMaker;

  struct RegionInfo
  {
    storage::TCountryId m_countryId;
    m2::PointD m_point;

    void SetParams(storage::TCountryId const & countryId, m2::PointD const & point)
    {
      m_countryId = countryId;
      m_point = point;
    }

    bool GetCountryId(storage::CountryInfoGetter const & infoGetter,
                      storage::TCountryId & countryId) const;
  };

  RegionInfo m_region;
  FeatureID m_id;
  feature::TypesHolder m_types;
  std::string m_str;
  double m_distance;
  Type m_resultType;
  RankingInfo m_info;
  feature::EGeomType m_geomType;
  Result::Metadata m_metadata;
};

void ProcessMetadata(FeatureType & ft, Result::Metadata & meta);

std::string DebugPrint(RankerResult const & r);
}  // namespace search

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
  PreRankerResult(FeatureID const & fID, PreRankingInfo const & info);

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

  inline search::RankingInfo const & GetRankingInfo() const { return m_info; }

  template <typename TInfo>
  inline void SetRankingInfo(TInfo && info)
  {
    m_info = forward<TInfo>(info);
  }

  string DebugPrint() const;

  bool IsStreet() const;

  inline FeatureID const & GetID() const { return m_id; }
  inline string const & GetName() const { return m_str; }
  inline feature::TypesHolder const & GetTypes() const { return m_types; }
  inline Type const & GetResultType() const { return m_resultType; }
  inline m2::PointD GetCenter() const { return m_region.m_point; }
  inline double GetDistance() const { return m_distance; }
  inline feature::EGeomType GetGeomType() const { return m_geomType; }
  inline Result::Metadata GetMetadata() const { return m_metadata; }

  inline double GetDistanceToPivot() const { return m_info.m_distanceToPivot; }
  inline double GetLinearModelRank() const { return m_info.GetLinearModelRank(); }

  string GetRegionName(storage::CountryInfoGetter const & infoGetter, uint32_t fType) const;

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

inline string DebugPrint(RankerResult const & t) { return t.DebugPrint(); }

void ProcessMetadata(FeatureType const & ft, Result::Metadata & meta);
}  // namespace search

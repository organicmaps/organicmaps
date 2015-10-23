#pragma once
#include "result.hpp"

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
namespace impl
{

template <class T> bool LessRankT(T const & r1, T const & r2);
template <class T> bool LessDistanceT(T const & r1, T const & r2);

/// First pass results class. Objects are creating during search in trie.
/// Works fast without feature loading and provide ranking.
class PreResult1
{
  friend class PreResult2;
  template <class T> friend bool LessRankT(T const & r1, T const & r2);
  template <class T> friend bool LessDistanceT(T const & r1, T const & r2);

  FeatureID m_id;
  m2::PointD m_center;
  double m_distance;
  uint8_t m_rank;
  int8_t m_viewportID;

  void CalcParams(m2::PointD const & pivot);

public:
  PreResult1(FeatureID const & fID, uint8_t rank, m2::PointD const & center,
             m2::PointD const & pivot, int8_t viewportID);
  PreResult1(m2::PointD const & center, m2::PointD const & pivot);

  static bool LessRank(PreResult1 const & r1, PreResult1 const & r2);
  static bool LessDistance(PreResult1 const & r1, PreResult1 const & r2);
  static bool LessPointsForViewport(PreResult1 const & r1, PreResult1 const & r2);

  inline FeatureID GetID() const { return m_id; }
  inline m2::PointD GetCenter() const { return m_center; }
  inline uint8_t GetRank() const { return m_rank; }
  inline int8_t GetViewportID() const { return m_viewportID; }
};


/// Second result class. Objects are creating during reading of features.
/// Read and fill needed info for ranking and getting final results.
class PreResult2
{
  friend class PreResult2Maker;

  void CalcParams(m2::PointD const & pivot);

public:
  enum ResultType
  {
    RESULT_LATLON,
    RESULT_FEATURE,
    RESULT_BUILDING
  };

  /// For RESULT_FEATURE.
  PreResult2(FeatureType const & f, PreResult1 const * p, m2::PointD const & pivot,
             string const & displayName, string const & fileName);

  /// For RESULT_LATLON.
  PreResult2(double lat, double lon);

  /// For RESULT_BUILDING.
  PreResult2(m2::PointD const & pt, string const & str, uint32_t type);

  /// @param[in]  infoGetter Need to get region for result.
  /// @param[in]  pCat    Categories need to display readable type string.
  /// @param[in]  pTypes  Set of preffered types that match input tokens by categories.
  /// @param[in]  lang    Current system language.
  Result GenerateFinalResult(storage::CountryInfoGetter const & infoGetter,
                             CategoriesHolder const * pCat, set<uint32_t> const * pTypes,
                             int8_t locale) const;

  Result GeneratePointResult(CategoriesHolder const * pCat, set<uint32_t> const * pTypes,
                             int8_t locale) const;

  static bool LessRank(PreResult2 const & r1, PreResult2 const & r2);
  static bool LessDistance(PreResult2 const & r1, PreResult2 const & r2);

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
  template <class T> friend bool LessRankT(T const & r1, T const & r2);
  template <class T> friend bool LessDistanceT(T const & r1, T const & r2);

  bool IsEqualCommon(PreResult2 const & r) const;

  string ReadableFeatureType(CategoriesHolder const * pCat,
                             uint32_t type, int8_t locale) const;

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
  uint8_t m_rank;
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

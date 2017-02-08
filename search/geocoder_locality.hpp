#pragma once

#include "search/model.hpp"
#include "search/token_range.hpp"

#include "indexer/mwm_set.hpp"

#include "storage/country_info_getter.hpp"

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"

#include <cstdint>
#include <string>

namespace search
{
struct Locality
{
  Locality() = default;

  Locality(MwmSet::MwmId const & countryId, uint32_t featureId, TokenRange const & tokenRange,
           double prob)
    : m_countryId(countryId), m_featureId(featureId), m_tokenRange(tokenRange), m_prob(prob)
  {
  }

  MwmSet::MwmId m_countryId;
  uint32_t m_featureId = 0;
  TokenRange m_tokenRange;

  // Measures our belief in the fact that tokens in the range
  // [m_startToken, m_endToken) indeed specify a locality. Currently
  // it is set only for villages.
  double m_prob = 0.0;
};

// This struct represents a country or US- or Canadian- state.  It
// is used to filter maps before search.
struct Region : public Locality
{
  enum Type
  {
    TYPE_STATE,
    TYPE_COUNTRY,
    TYPE_COUNT
  };

  Region(Locality const & locality, Type type) : Locality(locality), m_center(0, 0), m_type(type) {}

  static SearchModel::SearchType ToSearchType(Type type);

  storage::CountryInfoGetter::TRegionIdSet m_ids;
  std::string m_defaultName;
  m2::PointD m_center;
  Type m_type;
};

// This struct represents a city or a village. It is used to filter features
// during search.
// todo(@m) It works well as is, but consider a new naming scheme
// when counties etc. are added. E.g., Region for countries and
// states and Locality for smaller settlements.
struct City : public Locality
{
  City(Locality const & locality, SearchModel::SearchType type) : Locality(locality), m_type(type)
  {
  }

  m2::RectD m_rect;
  SearchModel::SearchType m_type;

#if defined(DEBUG)
  std::string m_defaultName;
#endif
};

std::string DebugPrint(Locality const & locality);
}  // namespace search

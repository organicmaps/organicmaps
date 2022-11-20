#pragma once

#include "search/doc_vec.hpp"
#include "search/model.hpp"
#include "search/token_range.hpp"

#include "indexer/feature_decl.hpp"
#include "indexer/mwm_set.hpp"

#include "storage/country_info_getter.hpp"

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"

#include <cstdint>
#include <string>

namespace search
{
class IdfMap;

struct Locality
{
  Locality(MwmSet::MwmId const & countryId, uint32_t featureId, TokenRange const & tokenRange,
           QueryVec const & queryVec, bool exactMatch)
    : m_countryId(countryId)
    , m_featureId(featureId)
    , m_tokenRange(tokenRange)
    , m_queryVec(queryVec)
    , m_exactMatch(exactMatch)
  {
  }

  double QueryNorm() { return m_queryVec.Norm(); }

  MwmSet::MwmId m_countryId;
  uint32_t m_featureId = 0;
  TokenRange m_tokenRange;
  QueryVec m_queryVec;
  bool m_exactMatch;
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

  Region(Locality && locality, Type type) : Locality(std::move(locality)), m_center(0, 0), m_type(type) {}

  static Model::Type ToModelType(Type type);

  storage::CountryInfoGetter::RegionIdVec m_ids;
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
  City(Locality && locality, Model::Type type) : Locality(std::move(locality)), m_type(type)
  {
  }

  m2::RectD m_rect;
  Model::Type m_type;
};

struct Suburb
{
  Suburb(FeatureID const & featureId, TokenRange const & tokenRange)
    : m_featureId(featureId), m_tokenRange(tokenRange)
  {
  }

  FeatureID m_featureId;
  TokenRange m_tokenRange;
};

std::string DebugPrint(Locality const & locality);
}  // namespace search

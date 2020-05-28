#pragma once

#include "generator/cities_boundaries_builder.hpp"
#include "generator/feature_builder.hpp"

#include "geometry/rect2d.hpp"

#include "base/geo_object_id.hpp"

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace generator
{
bool NeedProcessPlace(feature::FeatureBuilder const & fb);

// This structure encapsulates work with elements of different types.
// This means that we can consider a set of polygons of one relation as a single entity.
class FeaturePlace
{
public:
  using FeaturesBuilders = std::vector<feature::FeatureBuilder>;

  void Append(feature::FeatureBuilder const & fb);
  feature::FeatureBuilder const & GetFb() const;
  FeaturesBuilders const & GetFbs() const;

  // Returns limit rect around all stored feature builders.
  m2::RectD const & GetAllFbsLimitRect() const;

  // Methods return values for best stored feature builder.
  base::GeoObjectId GetMostGenericOsmId() const;
  uint8_t GetRank() const;
  std::string GetName() const;
  StringUtf8Multilang const & GetMultilangName() const;
  bool IsPoint() const;
  m2::RectD const & GetLimitRect() const;

private:
  m2::RectD m_allFbsLimitRect;
  FeaturesBuilders m_fbs;
  size_t m_bestIndex;
};

m2::RectD GetLimitRect(FeaturePlace const & fp);

// The class PlaceProcessor is responsible for the union of boundaries of the places.
class PlaceProcessor
{
public:
  using PlaceWithIds = std::pair<feature::FeatureBuilder, std::vector<base::GeoObjectId>>;

  PlaceProcessor(std::shared_ptr<OsmIdToBoundariesTable> boundariesTable = {});

  void Add(feature::FeatureBuilder const & fb);
  std::vector<PlaceWithIds> ProcessPlaces();

private:
  using FeaturePlaces = std::vector<FeaturePlace>;

  static std::string GetKey(feature::FeatureBuilder const & fb);
  void FillTable(FeaturePlaces::const_iterator start, FeaturePlaces::const_iterator end,
                 FeaturePlaces::const_iterator best) const;

  std::unordered_map<std::string, std::unordered_map<base::GeoObjectId, FeaturePlace>> m_nameToPlaces;
  std::shared_ptr<OsmIdToBoundariesTable> m_boundariesTable;
};
} // namespace generator

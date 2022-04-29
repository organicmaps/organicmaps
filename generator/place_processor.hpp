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

  void Append(feature::FeatureBuilder && fb);
  feature::FeatureBuilder const & GetFb() const;
  FeaturesBuilders const & GetFbs() const { return m_fbs; }

  /// @return limit rect around all stored feature builders.
  m2::RectD const & GetAllFbsLimitRect() const { return m_allFbsLimitRect; }

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
  PlaceProcessor(std::shared_ptr<OsmIdToBoundariesTable> boundariesTable = {});

  void Add(feature::FeatureBuilder && fb);

  using IDsContainerT = std::vector<base::GeoObjectId>;
  /// @param[out] ids To store correspondent IDs for test purposes if not nullptr.
  std::vector<feature::FeatureBuilder> ProcessPlaces(std::vector<IDsContainerT> * ids = nullptr);

private:
  using FeaturePlaces = std::vector<FeaturePlace>;

  template <class IterT> void FillTable(IterT start, IterT end, IterT best) const;

  std::unordered_map<std::string, std::unordered_map<base::GeoObjectId, FeaturePlace>> m_nameToPlaces;
  std::shared_ptr<OsmIdToBoundariesTable> m_boundariesTable;
};
} // namespace generator

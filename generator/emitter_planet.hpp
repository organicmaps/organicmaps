#pragma once

#include "generator/booking_dataset.hpp"
#include "generator/cities_boundaries_builder.hpp"
#include "generator/coastlines_generator.hpp"
#include "generator/emitter_interface.hpp"
#include "generator/feature_generator.hpp"
#include "generator/generate_info.hpp"
#include "generator/opentable_dataset.hpp"
#include "generator/place.hpp"
#include "generator/polygonizer.hpp"
#include "generator/viator_dataset.hpp"
#include "generator/world_map_generator.hpp"

#include "indexer/feature_data.hpp"

#include "base/geo_object_id.hpp"

#include <cstdint>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>

namespace generator
{
class EmitterPlanet : public EmitterInterface
{
  using WorldGenerator = WorldMapGenerator<feature::FeaturesCollector>;
  using CountriesGenerator = CountryMapGenerator<feature::Polygonizer<feature::FeaturesCollector>>;

public:
  explicit EmitterPlanet(feature::GenerateInfo const & info);

  // EmitterInterface overrides:
  void operator()(FeatureBuilder1 & fb) override;
  void EmitCityBoundary(FeatureBuilder1 const & fb, FeatureParams const & params) override;
  /// @return false if coasts are not merged and FLAG_fail_on_coasts is set
  bool Finish() override;
  void GetNames(std::vector<std::string> & names) const override;

private:
  enum TypeIndex
  {
    NATURAL_COASTLINE,
    NATURAL_LAND,
    PLACE_ISLAND,
    PLACE_ISLET,

    TYPES_COUNT
  };

  void Emit(FeatureBuilder1 & fb);
  void DumpSkippedElements();
  uint32_t Type(TypeIndex i) const { return m_types[i]; }
  uint32_t GetPlaceType(FeatureParams const & params) const;
  void UnionEqualPlacesIds(Place const & place);

  uint32_t m_types[TYPES_COUNT];
  std::unique_ptr<CountriesGenerator> m_countries;
  std::unique_ptr<WorldGenerator> m_world;
  std::unique_ptr<CoastlineFeaturesGenerator> m_coasts;
  std::unique_ptr<feature::FeaturesCollector> m_coastsHolder;
  std::string const m_skippedElementsPath;
  std::ostringstream m_skippedElements;
  std::string m_srcCoastsFile;
  bool m_failOnCoasts;
  BookingDataset m_bookingDataset;
  OpentableDataset m_opentableDataset;
  ViatorDataset m_viatorDataset;
  std::unordered_map<base::GeoObjectId, std::string> m_brands;
  shared_ptr<OsmIdToBoundariesTable> m_boundariesTable;
  /// Used to prepare a list of cities to serve as a list of nodes
  /// for building a highway graph with OSRM for low zooms.
  m4::Tree<Place> m_places;
};
}  // namespace generator

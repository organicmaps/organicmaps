#pragma once

#include "generator/coastlines_generator.hpp"
#include "generator/collector_building_parts.hpp"
#include "generator/feature_builder.hpp"
#include "generator/feature_generator.hpp"
#include "generator/filter_interface.hpp"
#include "generator/hierarchy.hpp"
#include "generator/hierarchy_entry.hpp"
#include "generator/world_map_generator.hpp"

#include <cstddef>
#include <cstdint>
#include <string>

namespace generator
{
enum class FinalProcessorPriority : uint8_t
{
  CountriesOrWorld = 1,
  WorldCoasts = 2,
  Complex = 3
};

// Classes that inherit this interface implement the final stage of intermediate mwm processing.
// For example, attempt to merge the coastline or adding external elements.
// Each derived class has a priority. This is done to comply with the order of processing
// intermediate mwm, taking into account the dependencies between them. For example, before adding a
// coastline to a country, we must build coastline. Processors with higher priority will be called
// first. Processors with the same priority can run in parallel.
class FinalProcessorIntermediateMwmInterface
{
public:
  explicit FinalProcessorIntermediateMwmInterface(FinalProcessorPriority priority);
  virtual ~FinalProcessorIntermediateMwmInterface() = default;

  virtual void Process() = 0;

  bool operator<(FinalProcessorIntermediateMwmInterface const & other) const;
  bool operator==(FinalProcessorIntermediateMwmInterface const & other) const;
  bool operator!=(FinalProcessorIntermediateMwmInterface const & other) const;

protected:
  FinalProcessorPriority m_priority;
};

class CountryFinalProcessor : public FinalProcessorIntermediateMwmInterface
{
public:
  CountryFinalProcessor(std::string const & borderPath,
                        std::string const & temporaryMwmPath,
                        bool haveBordersForWholeWorld, size_t threadsCount);

  void SetBooking(std::string const & filename);
  void SetCitiesAreas(std::string const & filename);
  void SetPromoCatalog(std::string const & filename);
  void SetCoastlines(std::string const & coastlineGeomFilename,
                     std::string const & worldCoastsFilename);
  void SetFakeNodes(std::string const & filename);
  void SetMiniRoundabouts(std::string const & filename);
  void SetIsolinesDir(std::string const & dir);

  void DumpCitiesBoundaries(std::string const & filename);
  void DumpRoutingCitiesBoundaries(std::string const & collectorFilename,
                                   std::string const & dumpPath);

  // FinalProcessorIntermediateMwmInterface overrides:
  void Process() override;

private:
  void ProcessBooking();
  void ProcessRoutingCityBoundaries();
  void ProcessCities();
  void ProcessCoastline();
  void ProcessRoundabouts();
  void AddFakeNodes();
  void AddIsolines();
  void Finish();

  std::string m_borderPath;
  std::string m_temporaryMwmPath;
  std::string m_isolinesPath;
  std::string m_citiesAreasTmpFilename;
  std::string m_citiesBoundariesFilename;
  std::string m_hotelsFilename;
  std::string m_coastlineGeomFilename;
  std::string m_worldCoastsFilename;
  std::string m_citiesFilename;
  std::string m_fakeNodesFilename;
  std::string m_miniRoundaboutsFilename;

  std::string m_routingCityBoundariesCollectorFilename;
  std::string m_routingCityBoundariesDumpPath;
  std::string m_hierarchySrcFilename;

  bool m_haveBordersForWholeWorld;
  size_t m_threadsCount;
};

class WorldFinalProcessor : public FinalProcessorIntermediateMwmInterface
{
public:
  using WorldGenerator = WorldMapGenerator<feature::FeaturesCollector>;

  explicit WorldFinalProcessor(std::string const & temporaryMwmPath,
                               std::string const & coastlineGeomFilename);

  void SetPopularPlaces(std::string const & filename);
  void SetCitiesAreas(std::string const & filename);
  void SetPromoCatalog(std::string const & filename);

  // FinalProcessorIntermediateMwmInterface overrides:
  void Process() override;

private:
  void ProcessCities();

  std::string m_temporaryMwmPath;
  std::string m_worldTmpFilename;
  std::string m_coastlineGeomFilename;
  std::string m_popularPlacesFilename;
  std::string m_citiesAreasTmpFilename;
  std::string m_citiesFilename;
};

class CoastlineFinalProcessor : public FinalProcessorIntermediateMwmInterface
{
public:
  explicit CoastlineFinalProcessor(std::string const & filename);

  void SetCoastlinesFilenames(std::string const & geomFilename,
                              std::string const & rawGeomFilename);

  // FinalProcessorIntermediateMwmInterface overrides:
  void Process() override;

private:
  std::string m_filename;
  std::string m_coastlineGeomFilename;
  std::string m_coastlineRawGeomFilename;
  CoastlineFeaturesGenerator m_generator;
};

// Class ComplexFinalProcessor generates hierarchies for each previously filtered mwm.tmp file.
// Warning: If the border separates the complex, then a situation is possible in which two logically
// identical complexes are generated, but with different representations.
class ComplexFinalProcessor : public FinalProcessorIntermediateMwmInterface
{
public:
  ComplexFinalProcessor(std::string const & mwmTmpPath, std::string const & outFilename,
                        size_t threadsCount);

  void SetGetMainTypeFunction(hierarchy::GetMainTypeFn const & getMainType);
  void SetFilter(std::shared_ptr<FilterInterface> const & filter);
  void SetGetNameFunction(hierarchy::GetNameFn const & getName);
  void SetPrintFunction(hierarchy::PrintFn const & printFunction);

  void UseCentersEnricher(std::string const & mwmPath, std::string const & osm2ftPath);
  void UseBuildingPartsInfo(std::string const & filename);

  // FinalProcessorIntermediateMwmInterface overrides:
  void Process() override;

private:
  std::unique_ptr<hierarchy::HierarchyEntryEnricher> CreateEnricher(
      std::string const & countryName) const;
  void WriteLines(std::vector<HierarchyEntry> const & lines);
  std::unordered_map<base::GeoObjectId, feature::FeatureBuilder> RemoveRelationBuildingParts(
      std::vector<feature::FeatureBuilder> & fbs);

  hierarchy::GetMainTypeFn m_getMainType;
  hierarchy::PrintFn m_printFunction;
  hierarchy::GetNameFn m_getName;
  std::shared_ptr<FilterInterface> m_filter;
  std::unique_ptr<BuildingToBuildingPartsMap> m_buildingToParts;
  bool m_useCentersEnricher = false;
  std::string m_mwmTmpPath;
  std::string m_outFilename;
  std::string m_mwmPath;
  std::string m_osm2ftPath;
  std::string m_buildingPartsFilename;
  size_t m_threadsCount;
};
}  // namespace generator

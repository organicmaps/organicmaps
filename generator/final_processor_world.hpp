#pragma once

#include "generator/feature_generator.hpp"
#include "generator/final_processor_interface.hpp"
#include "generator/world_map_generator.hpp"

#include <string>

namespace generator
{
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
}  // namespace generator

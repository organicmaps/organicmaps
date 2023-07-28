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

  /// @param[in]  coastGeomFilename   Can be empty if you don't care about cutting borders by water.
  WorldFinalProcessor(std::string const & temporaryMwmPath, std::string const & coastGeomFilename);

  void SetPopularPlaces(std::string const & filename);
  void SetCitiesAreas(std::string const & filename);

  // FinalProcessorIntermediateMwmInterface overrides:
  void Process() override;

private:
  void ProcessCities();

  std::string m_temporaryMwmPath;
  std::string m_worldTmpFilename;
  std::string m_coastlineGeomFilename;
  std::string m_popularPlacesFilename;
  std::string m_citiesAreasTmpFilename;
};
}  // namespace generator

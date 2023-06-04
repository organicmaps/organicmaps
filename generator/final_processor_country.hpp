#pragma once

#include "generator/final_processor_interface.hpp"
#include "generator/place_processor.hpp"

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace feature
{
class AffiliationInterface;
}  // namespace feature

namespace generator
{
class CountryFinalProcessor : public FinalProcessorIntermediateMwmInterface
{
public:
  CountryFinalProcessor(std::string const & borderPath, std::string const & temporaryMwmPath,
                        std::string const & intermediateDir, bool haveBordersForWholeWorld,
                        size_t threadsCount);

  void SetBooking(std::string const & filename);
  void SetCitiesAreas(std::string const & filename);
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
  void Order();
  void ProcessRoutingCityBoundaries();
  void ProcessCities();
  void ProcessCoastline();
  void ProcessRoundabouts();
  void AddFakeNodes();
  void AddIsolines();
  void DropProhibitedSpeedCameras();
  void Finish();
  void ProcessBuildingParts();

  bool IsCountry(std::string const & filename);

  std::string m_borderPath;
  std::string m_temporaryMwmPath;
  std::string m_intermediateDir;
  std::string m_isolinesPath;
  std::string m_citiesAreasTmpFilename;
  std::string m_citiesBoundariesFilename;
  std::string m_coastlineGeomFilename;
  std::string m_worldCoastsFilename;
  std::string m_citiesFilename;
  std::string m_fakeNodesFilename;
  std::string m_miniRoundaboutsFilename;

  std::string m_routingCityBoundariesCollectorFilename;
  std::string m_routingCityBoundariesDumpPath;
  std::string m_hierarchySrcFilename;

  std::unique_ptr<feature::AffiliationInterface> m_affiliations;

  size_t m_threadsCount;
};
}  // namespace generator

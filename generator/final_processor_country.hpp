#pragma once

#include "generator/affiliation.hpp"
#include "generator/final_processor_interface.hpp"

#include <string>

namespace generator
{
class CountryFinalProcessor : public FinalProcessorIntermediateMwmInterface
{
public:
  CountryFinalProcessor(AffiliationInterfacePtr affiliations,
                        std::string const & temporaryMwmPath, size_t threadsCount);

  void SetBooking(std::string const & filename);
  void SetCoastlines(std::string const & coastlineGeomFilename,
                     std::string const & worldCoastsFilename);
  void SetFakeNodes(std::string const & filename);
  void SetMiniRoundabouts(std::string const & filename);
  void SetAddrInterpolation(std::string const & filename);
  void SetIsolinesDir(std::string const & dir);

  void SetCityBoundariesFiles(std::string const & collectorFile)
  {
    m_boundariesCollectorFile = collectorFile;
  }

  // FinalProcessorIntermediateMwmInterface overrides:
  void Process() override;

  void ProcessBuildingParts();

private:
  //void Order();
  void ProcessCoastline();
  void ProcessRoundabouts();
  void AddFakeNodes();
  void AddIsolines();
  void DropProhibitedSpeedCameras();
  //void Finish();

  bool IsCountry(std::string const & filename);

  std::string m_borderPath;
  std::string m_temporaryMwmPath;
  std::string m_intermediateDir;
  std::string m_isolinesPath;
  std::string m_boundariesCollectorFile;
  std::string m_coastlineGeomFilename;
  std::string m_worldCoastsFilename;
  std::string m_fakeNodesFilename;
  std::string m_miniRoundaboutsFilename;
  std::string m_addrInterpolFilename;

  std::string m_hierarchySrcFilename;

  AffiliationInterfacePtr m_affiliations;

  size_t m_threadsCount;
};
}  // namespace generator

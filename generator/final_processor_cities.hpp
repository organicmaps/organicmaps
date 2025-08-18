#pragma once
#include "generator/affiliation.hpp"
#include "generator/final_processor_interface.hpp"

namespace generator
{

class FinalProcessorCities : public FinalProcessorIntermediateMwmInterface
{
public:
  FinalProcessorCities(AffiliationInterfacePtr const & affiliation, std::string const & mwmPath,
                       size_t threadsCount = 1);

  void SetCityBoundariesFiles(std::string const & collectorFile, std::string const & boundariesOutFile)
  {
    m_boundariesCollectorFile = collectorFile;
    m_boundariesOutFile = boundariesOutFile;
  }

  void Process();

private:
  std::string m_temporaryMwmPath, m_boundariesCollectorFile, m_boundariesOutFile;
  AffiliationInterfacePtr m_affiliation;
  size_t m_threadsCount;
};

}  // namespace generator

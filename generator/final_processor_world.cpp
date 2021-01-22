#include "generator/final_processor_world.hpp"

#include "generator/affiliation.hpp"
#include "generator/feature_builder.hpp"
#include "generator/final_processor_utils.hpp"

#include "defines.hpp"

using namespace feature;

namespace generator
{
WorldFinalProcessor::WorldFinalProcessor(std::string const & temporaryMwmPath,
                                         std::string const & coastlineGeomFilename)
  : FinalProcessorIntermediateMwmInterface(FinalProcessorPriority::CountriesOrWorld)
  , m_temporaryMwmPath(temporaryMwmPath)
  , m_worldTmpFilename(base::JoinPath(m_temporaryMwmPath, WORLD_FILE_NAME) +
                       DATA_FILE_EXTENSION_TMP)
  , m_coastlineGeomFilename(coastlineGeomFilename)
{
}

void WorldFinalProcessor::SetPopularPlaces(std::string const & filename)
{
  m_popularPlacesFilename = filename;
}

void WorldFinalProcessor::SetCitiesAreas(std::string const & filename)
{
  m_citiesAreasTmpFilename = filename;
}

void WorldFinalProcessor::SetPromoCatalog(std::string const & filename)
{
  m_citiesFilename = filename;
}

void WorldFinalProcessor::Process()
{
  if (!m_citiesAreasTmpFilename.empty() || !m_citiesFilename.empty())
    ProcessCities();

  auto fbs = ReadAllDatRawFormat<serialization_policy::MaxAccuracy>(m_worldTmpFilename);
  Order(fbs);
  WorldGenerator generator(m_worldTmpFilename, m_coastlineGeomFilename, m_popularPlacesFilename);
  for (auto & fb : fbs)
    generator.Process(fb);

  generator.DoMerge();
}

void WorldFinalProcessor::ProcessCities()
{
  auto const affiliation = SingleAffiliation(WORLD_FILE_NAME);
  auto citiesHelper =
      m_citiesAreasTmpFilename.empty() ? PlaceHelper() : PlaceHelper(m_citiesAreasTmpFilename);
  ProcessorCities processorCities(m_temporaryMwmPath, affiliation, citiesHelper);
  processorCities.SetPromoCatalog(m_citiesFilename);
  processorCities.Process();
}
}  // namespace generator

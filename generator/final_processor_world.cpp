#include "generator/final_processor_world.hpp"
#include "generator/feature_builder.hpp"
#include "generator/final_processor_utils.hpp"

#include "base/logging.hpp"

#include "defines.hpp"

namespace generator
{
using namespace feature;

WorldFinalProcessor::WorldFinalProcessor(std::string const & temporaryMwmPath,
                                         std::string const & coastlineGeomFilename)
  : FinalProcessorIntermediateMwmInterface(FinalProcessorPriority::CountriesOrWorld)
  , m_temporaryMwmPath(temporaryMwmPath)
  , m_worldTmpFilename(base::JoinPath(m_temporaryMwmPath, WORLD_FILE_NAME) + DATA_FILE_EXTENSION_TMP)
  , m_coastlineGeomFilename(coastlineGeomFilename)
{}

void WorldFinalProcessor::Process()
{
  auto fbs = ReadAllDatRawFormat<serialization_policy::MaxAccuracy>(m_worldTmpFilename);
  Order(fbs);
  WorldGenerator generator(m_worldTmpFilename, m_coastlineGeomFilename, m_popularPlacesFilename);
  LOG(LINFO, ("Process World features"));
  for (auto & fb : fbs)
    generator.Process(fb);

  LOG(LINFO, ("Merge World lines"));
  generator.DoMerge();
}

}  // namespace generator

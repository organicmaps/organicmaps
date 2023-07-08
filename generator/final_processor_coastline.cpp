#include "generator/final_processor_coastline.hpp"

#include "generator/feature_builder.hpp"
#include "generator/feature_generator.hpp"
#include "generator/final_processor_utils.hpp"

#include <vector>

using namespace feature;

namespace generator
{
CoastlineFinalProcessor::CoastlineFinalProcessor(std::string const & filename)
  : FinalProcessorIntermediateMwmInterface(FinalProcessorPriority::WorldCoasts)
  , m_filename(filename)
{
}

void CoastlineFinalProcessor::SetCoastlinesFilenames(std::string const & geomFilename,
                                                     std::string const & rawGeomFilename)
{
  m_coastlineGeomFilename = geomFilename;
  m_coastlineRawGeomFilename = rawGeomFilename;
}

void CoastlineFinalProcessor::Process()
{
  auto fbs = ReadAllDatRawFormat<serialization_policy::MaxAccuracy>(m_filename);
  Order(fbs);
  for (auto && fb : fbs)
    m_generator.Process(std::move(fb));

  FeaturesAndRawGeometryCollector collector(m_coastlineGeomFilename, m_coastlineRawGeomFilename);
  // Check and stop if some coasts were not merged.
  CHECK(m_generator.Finish(), ());
  LOG(LINFO, ("Generating coastline polygons."));
  size_t totalFeatures = 0;
  size_t totalPoints = 0;
  size_t totalPolygons = 0;
  std::vector<FeatureBuilder> outputFbs;
  m_generator.GetFeatures(outputFbs);
  for (auto & fb : outputFbs)
  {
    collector.Collect(fb);
    ++totalFeatures;
    totalPoints += fb.GetPointsCount();
    totalPolygons += fb.GetPolygonsCount();
  }

  LOG(LINFO, ("Total features:", totalFeatures, "total polygons:", totalPolygons,
              "total points:", totalPoints));
}
}  // namespace generator

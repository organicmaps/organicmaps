#include "generator/final_processor_coastline.hpp"

#include "generator/feature_builder.hpp"
#include "generator/feature_generator.hpp"

namespace generator
{
using namespace feature;

CoastlineFinalProcessor::CoastlineFinalProcessor(std::string const & filename, size_t threadsCount)
  : FinalProcessorIntermediateMwmInterface(FinalProcessorPriority::WorldCoasts)
  , m_filename(filename)
  , m_threadsCount(threadsCount)
{}

void CoastlineFinalProcessor::SetCoastlinesFilenames(std::string const & geomFilename,
                                                     std::string const & rawGeomFilename)
{
  m_coastlineGeomFilename = geomFilename;
  m_coastlineRawGeomFilename = rawGeomFilename;
}

void CoastlineFinalProcessor::Process()
{
  ForEachFeatureRawFormat<serialization_policy::MaxAccuracy>(
      m_filename, [this](FeatureBuilder const & fb, uint64_t) { m_generator.Process(fb); });

  FeaturesAndRawGeometryCollector collector(m_coastlineGeomFilename, m_coastlineRawGeomFilename);
  // Check and stop if some coasts were not merged.
  CHECK(m_generator.Finish(), ());

  LOG(LINFO, ("Generating coastline polygons."));
  size_t totalFeatures = 0;
  size_t totalPoints = 0;
  size_t totalPolygons = 0;
  for (auto const & fb : m_generator.GetFeatures(m_threadsCount))
  {
    collector.Collect(fb);
    ++totalFeatures;
    totalPoints += fb.GetPointsCount();
    totalPolygons += fb.GetPolygonsCount();
  }

  LOG(LINFO, ("Total features:", totalFeatures, "total polygons:", totalPolygons, "total points:", totalPoints));
}
}  // namespace generator

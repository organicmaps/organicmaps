#include "generator/emitter_coastline.hpp"

#include "generator/coastlines_generator.hpp"
#include "generator/feature_builder.hpp"
#include "generator/feature_processing_layers.hpp"
#include "generator/feature_generator.hpp"
#include "generator/generate_info.hpp"
#include "generator/type_helper.hpp"

#include "base/logging.hpp"

#include <cstddef>

#include "defines.hpp"

namespace generator
{
EmitterCoastline::EmitterCoastline(feature::GenerateInfo const & info)
  : m_generator(std::make_shared<CoastlineFeaturesGenerator>(ftypes::IsCoastlineChecker::Instance().GetCoastlineType()))
  , m_coastlineGeomFilename(info.GetIntermediateFileName(WORLD_COASTS_FILE_NAME, ".geom"))
  , m_coastlineRawGeomFilename(info.GetIntermediateFileName(WORLD_COASTS_FILE_NAME, RAW_GEOM_FILE_EXTENSION))
{
  m_processingChain = std::make_shared<RepresentationCoastlineLayer>();
  m_processingChain->Add(std::make_shared<PrepareCoastlineFeatureLayer>());
  m_processingChain->Add(std::make_shared<CoastlineMapperLayer>(m_generator));
}

void EmitterCoastline::Process(FeatureBuilder1 & feature)
{
  m_processingChain->Handle(feature);
}

bool EmitterCoastline::Finish()
{
  feature::FeaturesAndRawGeometryCollector collector(m_coastlineGeomFilename, m_coastlineRawGeomFilename);
  // Check and stop if some coasts were not merged
  if (!m_generator->Finish())
    return false;

  LOG(LINFO, ("Generating coastline polygons"));

  size_t totalFeatures = 0;
  size_t totalPoints = 0;
  size_t totalPolygons = 0;

  vector<FeatureBuilder1> features;
  m_generator->GetFeatures(features);
  for (auto & feature : features)
  {
    collector(feature);

    ++totalFeatures;
    totalPoints += feature.GetPointsCount();
    totalPolygons += feature.GetPolygonsCount();
  }

  LOG(LINFO, ("Total features:", totalFeatures, "total polygons:", totalPolygons,
              "total points:", totalPoints));

  return true;
}

void EmitterCoastline::GetNames(std::vector<std::string> &) const {}
}  // namespace generator

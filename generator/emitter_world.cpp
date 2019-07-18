#include "generator/emitter_world.hpp"

#include "generator/cities_boundaries_builder.hpp"
#include "generator/feature_builder.hpp"
#include "generator/feature_processing_layers.hpp"
#include "generator/generate_info.hpp"
#include "generator/place_processor.hpp"

#include "defines.hpp"

using namespace feature;

namespace generator
{
EmitterWorld::EmitterWorld(feature::GenerateInfo const & info)
  : m_placeProcessor(std::make_shared<PlaceProcessor>(
        std::make_shared<generator::OsmIdToBoundariesTable>()))
  , m_worldMapper(std::make_shared<WorldMapper>(
        info.GetTmpFileName(WORLD_FILE_NAME),
        info.GetIntermediateFileName(WORLD_COASTS_FILE_NAME, RAW_GEOM_FILE_EXTENSION),
        info.m_popularPlacesFilename))
{
  m_processingChain = std::make_shared<RepresentationLayer>(m_placeProcessor);
  m_processingChain->Add(std::make_shared<PrepareFeatureLayer>());
  m_processingChain->Add(std::make_shared<PromoCatalogLayer>(info.m_promoCatalogCitiesFilename));
  m_processingChain->Add(std::make_shared<PlaceLayer>(m_placeProcessor));
  m_processingChain->Add(std::make_shared<WorldAreaLayer>(m_worldMapper));
}

void EmitterWorld::Process(FeatureBuilder & feature)
{
  m_processingChain->Handle(feature);
}

bool EmitterWorld::Finish()
{
  for (auto & feature : m_placeProcessor->GetFeatures())
    m_worldMapper->RemoveInvalidTypesAndMap(feature);

  return true;
}

void EmitterWorld::GetNames(vector<string> &) const {}
}  // namespace generator

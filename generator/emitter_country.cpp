#include "generator/emitter_country.hpp"

#include "generator/feature_builder.hpp"
#include "generator/feature_processing_layers.hpp"
#include "generator/generate_info.hpp"
#include "generator/place_processor.hpp"

#include "base/logging.hpp"

#include <fstream>

#include "defines.hpp"

using namespace feature;

namespace generator
{
EmitterCountry::EmitterCountry(feature::GenerateInfo const & info)
  : m_placeProcessor(std::make_shared<PlaceProcessor>(info.m_boundariesTable))
  , m_countryMapper(std::make_shared<CountryMapper>(info))
  , m_skippedListFilename(info.GetIntermediateFileName("skipped_elements", ".lst"))
{
  m_processingChain = std::make_shared<RepresentationLayer>(m_placeProcessor);
  m_processingChain->Add(std::make_shared<PrepareFeatureLayer>());
  m_processingChain->Add(std::make_shared<PromoCatalogLayer>(info.m_promoCatalogCitiesFilename));
  m_processingChain->Add(std::make_shared<PlaceLayer>(m_placeProcessor));
  m_processingChain->Add(std::make_shared<BookingLayer>(info.m_bookingDataFilename, m_countryMapper));
  m_processingChain->Add(std::make_shared<OpentableLayer>(info.m_opentableDataFilename, m_countryMapper));
  m_processingChain->Add(std::make_shared<CountryMapperLayer>(m_countryMapper));

  if (info.m_emitCoasts)
  {
    auto const geomFilename = info.GetIntermediateFileName(WORLD_COASTS_FILE_NAME, ".geom");
    auto const worldCoastsFilename = info.GetTmpFileName(WORLD_COASTS_FILE_NAME);
    m_processingChain->Add(std::make_shared<EmitCoastsLayer>(worldCoastsFilename, geomFilename, m_countryMapper));
  }
}

void EmitterCountry::Process(FeatureBuilder & feature)
{
  m_processingChain->Handle(feature);
}

bool EmitterCountry::Finish()
{
  for (auto & feature : m_placeProcessor->GetFeatures())
    m_countryMapper->RemoveInvalidTypesAndMap(feature);

  WriteDump();
  return true;
}

void EmitterCountry::GetNames(std::vector<std::string> & names) const
{
  names = m_countryMapper->GetNames();
}

void EmitterCountry::WriteDump()
{
  std::ofstream file;
  file.exceptions(std::ios::failbit | std::ios::badbit);
  file.open(m_skippedListFilename);
  file << m_processingChain->GetAsStringRecursive();
  LOG(LINFO, ("Skipped elements were saved to", m_skippedListFilename));
}
}  // namespace generator

#include "generator/processor_country.hpp"

#include "generator/feature_builder.hpp"
#include "generator/generate_info.hpp"

#include "base/logging.hpp"

#include <fstream>

namespace generator
{
ProcessorCountry::ProcessorCountry(std::shared_ptr<FeatureProcessorQueue> const & queue,
                                   std::string const & bordersPath, bool haveBordersForWholeWorld,
                                   std::shared_ptr<ComplexFeaturesMixer> const & complexFeaturesMixer)
  : m_bordersPath(bordersPath)
  , m_queue(queue)
  , m_complexFeaturesMixer(complexFeaturesMixer)
  , m_haveBordersForWholeWorld(haveBordersForWholeWorld)
{
  m_processingChain = std::make_shared<RepresentationLayer>(m_complexFeaturesMixer);
  m_processingChain->Add(std::make_shared<PrepareFeatureLayer>());
  m_processingChain->Add(std::make_shared<CountryLayer>());
  auto affiliation = std::make_shared<feature::CountriesFilesIndexAffiliation>(
      bordersPath, haveBordersForWholeWorld);
  m_affiliationsLayer =
      std::make_shared<AffiliationsFeatureLayer<>>(kAffiliationsBufferSize, affiliation, m_queue);
  m_processingChain->Add(m_affiliationsLayer);
}

std::shared_ptr<FeatureProcessorInterface> ProcessorCountry::Clone() const
{
  return std::make_shared<ProcessorCountry>(m_queue, m_bordersPath, m_haveBordersForWholeWorld,
                                            m_complexFeaturesMixer->Clone());
}

void ProcessorCountry::Process(feature::FeatureBuilder & feature)
{
  m_processingChain->Handle(feature);
}

void ProcessorCountry::Finish() { m_affiliationsLayer->AddBufferToQueue(); }
}  // namespace generator

#include "generator/processor_country.hpp"

#include "generator/feature_builder.hpp"

namespace generator
{
ProcessorCountry::ProcessorCountry(AffiliationInterfacePtr affiliations, std::shared_ptr<FeatureProcessorQueue> queue)
  : m_affiliations(std::move(affiliations))
  , m_queue(std::move(queue))
{
  ASSERT(m_affiliations && m_queue, ());

  m_processingChain = std::make_shared<RepresentationLayer>();
  m_processingChain->Add(std::make_shared<PrepareFeatureLayer>());
  m_processingChain->Add(std::make_shared<CountryLayer>());

  m_affiliationsLayer = std::make_shared<AffiliationsFeatureLayer>(kAffiliationsBufferSize, m_affiliations, m_queue);
  m_processingChain->Add(m_affiliationsLayer);
}

std::shared_ptr<FeatureProcessorInterface> ProcessorCountry::Clone() const
{
  return std::make_shared<ProcessorCountry>(m_affiliations, m_queue);
}

void ProcessorCountry::Process(feature::FeatureBuilder & feature)
{
  m_processingChain->Handle(feature);
}

void ProcessorCountry::Finish()
{
  m_affiliationsLayer->AddBufferToQueue();
}
}  // namespace generator

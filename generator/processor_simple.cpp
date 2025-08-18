#include "generator/processor_simple.hpp"

#include "generator/affiliation.hpp"
#include "generator/feature_builder.hpp"

#include "base/macros.hpp"

namespace generator
{
ProcessorSimple::ProcessorSimple(std::shared_ptr<FeatureProcessorQueue> const & queue, std::string const & name)
  : m_name(name)
  , m_queue(queue)
{
  m_processingChain = std::make_shared<PreserializeLayer>();
  auto affiliation = std::make_shared<feature::SingleAffiliation>(name);
  m_affiliationsLayer = std::make_shared<AffiliationsFeatureLayer<feature::serialization_policy::MinSize>>(
      kAffiliationsBufferSize, affiliation, m_queue);
  m_processingChain->Add(m_affiliationsLayer);
}

std::shared_ptr<FeatureProcessorInterface> ProcessorSimple::Clone() const
{
  return std::make_shared<ProcessorSimple>(m_queue, m_name);
}

void ProcessorSimple::Process(feature::FeatureBuilder & fb)
{
  m_processingChain->Handle(fb);
}

void ProcessorSimple::Finish()
{
  m_affiliationsLayer->AddBufferToQueue();
}
}  // namespace generator

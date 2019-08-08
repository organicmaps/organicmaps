#include "generator/processor_simple.hpp"

#include "generator/affiliation.hpp"
#include "generator/feature_builder.hpp"

#include "base/macros.hpp"

namespace generator
{
ProcessorSimple::ProcessorSimple(std::shared_ptr<FeatureProcessorQueue> const & queue,
                                 std::string const & name)
  : m_name(name)
  , m_queue(queue)
{
  m_processingChain->Add(std::make_shared<PreserializeLayer>());
  auto affilation = std::make_shared<feature::SingleAffiliation>(name);
  m_affilationsLayer = std::make_shared<AffilationsFeatureLayer<feature::serialization_policy::MinSize>>(kAffilationsBufferSize, affilation);
  m_processingChain->Add(m_affilationsLayer);
}

std::shared_ptr<FeatureProcessorInterface>ProcessorSimple::Clone() const
{
  return std::make_shared<ProcessorSimple>(m_queue, m_name);
}

void ProcessorSimple::Process(feature::FeatureBuilder & fb)
{
  m_processingChain->Handle(fb);
  m_affilationsLayer->AddBufferToQueueIfFull(m_queue);
}

void ProcessorSimple::Finish()
{
  m_affilationsLayer->AddBufferToQueue(m_queue);
}

void ProcessorSimple::Merge(FeatureProcessorInterface const & other)
{
  other.MergeInto(*this);
}

void ProcessorSimple::MergeInto(ProcessorSimple & other) const
{
  other.m_processingChain->Merge(m_processingChain);
}
}  // namespace generator

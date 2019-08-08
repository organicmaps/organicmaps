#include "generator/processor_coastline.hpp"

#include "generator/coastlines_generator.hpp"
#include "generator/feature_builder.hpp"
#include "generator/feature_generator.hpp"
#include "generator/generate_info.hpp"
#include "generator/type_helper.hpp"

#include "base/logging.hpp"

#include <cstddef>

#include "defines.hpp"

namespace generator
{
ProcessorCoastline::ProcessorCoastline(std::shared_ptr<FeatureProcessorQueue> const & queue)
  : m_queue(queue)
{
  m_processingChain = std::make_shared<RepresentationCoastlineLayer>();
  m_processingChain->Add(std::make_shared<PrepareCoastlineFeatureLayer>());
  auto affilation = std::make_shared<feature::SingleAffiliation>(WORLD_COASTS_FILE_NAME);
  m_affilationsLayer = std::make_shared<AffilationsFeatureLayer<>>(kAffilationsBufferSize, affilation);
  m_processingChain->Add(m_affilationsLayer);
}

std::shared_ptr<FeatureProcessorInterface> ProcessorCoastline::Clone() const
{
  return std::make_shared<ProcessorCoastline>(m_queue);
}

void ProcessorCoastline::Process(feature::FeatureBuilder & feature)
{
  m_processingChain->Handle(feature);
  m_affilationsLayer->AddBufferToQueueIfFull(m_queue);
}

void ProcessorCoastline::Finish()
{
  m_affilationsLayer->AddBufferToQueue(m_queue);
}

void ProcessorCoastline::Merge(FeatureProcessorInterface const & other)
{
  other.MergeInto(*this);
}

void ProcessorCoastline::MergeInto(ProcessorCoastline & other) const
{
  other.m_processingChain->Merge(m_processingChain);
}
}  // namespace generator

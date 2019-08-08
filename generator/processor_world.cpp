#include "generator/processor_world.hpp"

#include "generator/cities_boundaries_builder.hpp"
#include "generator/feature_builder.hpp"
#include "generator/generate_info.hpp"

#include "defines.hpp"

namespace generator
{
ProcessorWorld::ProcessorWorld(std::shared_ptr<FeatureProcessorQueue> const & queue,
                               std::string const & popularityFilename)
  : m_popularityFilename(popularityFilename)
  , m_queue(queue)
{
  m_processingChain = std::make_shared<RepresentationLayer>();
  m_processingChain->Add(std::make_shared<PrepareFeatureLayer>());
  m_processingChain->Add(std::make_shared<WorldLayer>(popularityFilename));
  auto affilation = std::make_shared<feature::SingleAffiliation>(WORLD_FILE_NAME);
  m_affilationsLayer = std::make_shared<AffilationsFeatureLayer<>>(kAffilationsBufferSize, affilation);
  m_processingChain->Add(m_affilationsLayer);
}

std::shared_ptr<FeatureProcessorInterface> ProcessorWorld::Clone() const
{
  return std::make_shared<ProcessorWorld>(m_queue, m_popularityFilename);
}

void ProcessorWorld::Process(feature::FeatureBuilder & feature)
{
  m_processingChain->Handle(feature);
  m_affilationsLayer->AddBufferToQueueIfFull(m_queue);
}

void ProcessorWorld::Finish()
{
  m_affilationsLayer->AddBufferToQueue(m_queue);
}

void ProcessorWorld::Merge(FeatureProcessorInterface const & other)
{
  other.MergeInto(*this);
}

void ProcessorWorld::MergeInto(ProcessorWorld & other) const
{
  other.m_processingChain->Merge(m_processingChain);
}
}  // namespace generator

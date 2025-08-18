#include "generator/processor_world.hpp"

#include "generator/feature_builder.hpp"

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
  auto affiliation = std::make_shared<feature::SingleAffiliation>(WORLD_FILE_NAME);
  m_affiliationsLayer = std::make_shared<AffiliationsFeatureLayer>(kAffiliationsBufferSize, affiliation, m_queue);
  m_processingChain->Add(m_affiliationsLayer);
}

std::shared_ptr<FeatureProcessorInterface> ProcessorWorld::Clone() const
{
  return std::make_shared<ProcessorWorld>(m_queue, m_popularityFilename);
}

void ProcessorWorld::Process(feature::FeatureBuilder & feature)
{
  m_processingChain->Handle(feature);
}

void ProcessorWorld::Finish()
{
  m_affiliationsLayer->AddBufferToQueue();
}
}  // namespace generator

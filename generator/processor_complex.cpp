#include "generator/processor_complex.hpp"

#include "generator/feature_builder.hpp"
#include "generator/generate_info.hpp"

#include "base/logging.hpp"

#include <fstream>

namespace generator
{
ProcessorComplex::ProcessorComplex(std::shared_ptr<FeatureProcessorQueue> const & queue,
                                   std::string const & bordersPath, bool haveBordersForWholeWorld)
  : m_bordersPath(bordersPath)
  , m_queue(queue)
  , m_haveBordersForWholeWorld(haveBordersForWholeWorld)
{
  m_processingChain = std::make_shared<PrepareFeatureLayer>();
  auto affiliation = std::make_shared<feature::CountriesFilesIndexAffiliation>(bordersPath, haveBordersForWholeWorld);
  m_affiliationsLayer = std::make_shared<AffiliationsFeatureLayer<>>(kAffiliationsBufferSize, affiliation, m_queue);
  m_processingChain->Add(m_affiliationsLayer);
}

std::shared_ptr<FeatureProcessorInterface> ProcessorComplex::Clone() const
{
  return std::make_shared<ProcessorComplex>(m_queue, m_bordersPath, m_haveBordersForWholeWorld);
}

void ProcessorComplex::Process(feature::FeatureBuilder & feature)
{
  m_processingChain->Handle(feature);
}

void ProcessorComplex::Finish()
{
  m_affiliationsLayer->AddBufferToQueue();
}
}  // namespace generator

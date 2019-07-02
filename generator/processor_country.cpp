#include "generator/processor_country.hpp"

#include "generator/feature_builder.hpp"
#include "generator/generate_info.hpp"

#include "base/logging.hpp"

#include <fstream>

#include "defines.hpp"

namespace generator
{
ProcessorCountry::ProcessorCountry(std::shared_ptr<FeatureProcessorQueue> const & queue,
                                   std::string const & bordersPath, std::string const & layerLogFilename,
                                   bool isMwmsForWholeWorld)
  : m_bordersPath(bordersPath)
  , m_layerLogFilename(layerLogFilename)
  , m_queue(queue)
  , m_isMwmsForWholeWorld(isMwmsForWholeWorld)
{
  m_processingChain = std::make_shared<RepresentationLayer>();
  m_processingChain->Add(std::make_shared<PrepareFeatureLayer>());
  m_processingChain->Add(std::make_shared<CountryLayer>());
  auto affilation = std::make_shared<feature::CountriesFilesAffiliation>(bordersPath, isMwmsForWholeWorld);
  m_affilationsLayer = std::make_shared<AffilationsFeatureLayer<>>(kAffilationsBufferSize, affilation);
  m_processingChain->Add(m_affilationsLayer);
}


std::shared_ptr<FeatureProcessorInterface> ProcessorCountry::Clone() const
{
  return std::make_shared<ProcessorCountry>(m_queue, m_bordersPath, m_layerLogFilename, m_isMwmsForWholeWorld);
}

void ProcessorCountry::Process(feature::FeatureBuilder & feature)
{
  m_processingChain->Handle(feature);
  m_affilationsLayer->AddBufferToQueueIfFull(m_queue);
}

void ProcessorCountry::Flush()
{
  m_affilationsLayer->AddBufferToQueue(m_queue);
}

bool ProcessorCountry::Finish()
{
  //  WriteDump();
  return true;
}

void ProcessorCountry::WriteDump()
{
  std::ofstream file;
  file.exceptions(std::ios::failbit | std::ios::badbit);
  file.open(m_layerLogFilename);
  file << m_processingChain->GetAsStringRecursive();
  LOG(LINFO, ("Skipped elements were saved to", m_layerLogFilename));
}

void ProcessorCountry::Merge(FeatureProcessorInterface const & other)
{
  other.MergeInto(*this);
}

void ProcessorCountry::MergeInto(ProcessorCountry & other) const
{
  other.m_processingChain->Merge(m_processingChain);
}
}  // namespace generator

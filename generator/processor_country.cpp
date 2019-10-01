#include "generator/processor_country.hpp"

#include "generator/feature_builder.hpp"
#include "generator/generate_info.hpp"

#include "base/logging.hpp"

#include <fstream>

namespace generator
{
ProcessorCountry::ProcessorCountry(std::shared_ptr<FeatureProcessorQueue> const & queue,
                                   std::string const & bordersPath,
                                   std::string const & layerLogFilename,
                                   bool haveBordersForWholeWorld)
  : m_bordersPath(bordersPath)
  , m_layerLogFilename(layerLogFilename)
  , m_queue(queue)
  , m_haveBordersForWholeWorld(haveBordersForWholeWorld)
{
  m_processingChain = std::make_shared<RepresentationLayer>();
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
  return std::make_shared<ProcessorCountry>(m_queue, m_bordersPath, m_layerLogFilename,
                                            m_haveBordersForWholeWorld);
}

void ProcessorCountry::Process(feature::FeatureBuilder & feature)
{
  m_processingChain->Handle(feature);
}

void ProcessorCountry::Finish() { m_affiliationsLayer->AddBufferToQueue(); }

void ProcessorCountry::WriteDump()
{
  std::ofstream file;
  file.exceptions(std::ios::failbit | std::ios::badbit);
  file.open(m_layerLogFilename);
  file << m_processingChain->GetAsStringRecursive();
  LOG(LINFO, ("Skipped elements were saved to", m_layerLogFilename));
}

void ProcessorCountry::Merge(FeatureProcessorInterface const & other) { other.MergeInto(*this); }

void ProcessorCountry::MergeInto(ProcessorCountry & other) const
{
  other.m_processingChain->MergeChain(m_processingChain);
}
}  // namespace generator

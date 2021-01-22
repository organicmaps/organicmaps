#include "generator/translator.hpp"

#include "generator/collector_collection.hpp"
#include "generator/filter_collection.hpp"
#include "generator/osm_element.hpp"

#include "base/assert.hpp"

using namespace feature;

namespace generator
{
Translator::Translator(std::shared_ptr<FeatureProcessorInterface> const & processor,
                       std::shared_ptr<cache::IntermediateData> const & cache,
                       std::shared_ptr<FeatureMakerBase> const & maker,
                       std::shared_ptr<FilterInterface> const & filter,
                       std::shared_ptr<CollectorInterface> const & collector)
  : m_filter(filter)
  , m_collector(collector)
  , m_tagsEnricher(cache->GetCache())
  , m_featureMaker(maker)
  , m_processor(processor)
  , m_cache(cache)
{
  m_featureMaker->SetCache(cache->GetCache());
}

Translator::Translator(std::shared_ptr<FeatureProcessorInterface> const & processor,
                       std::shared_ptr<cache::IntermediateData> const & cache,
                       std::shared_ptr<FeatureMakerBase> const & maker)
  : Translator(processor, cache, maker, std::make_shared<FilterCollection>(),
               std::make_shared<CollectorCollection>())
{
}

void Translator::SetCollector(std::shared_ptr<CollectorInterface> const & collector)
{
  m_collector = collector;
}

void Translator::SetFilter(std::shared_ptr<FilterInterface> const & filter) { m_filter = filter; }

void Translator::Emit(OsmElement & element)
{
  Preprocess(element);
  if (!m_filter->IsAccepted(element))
    return;

  m_tagsEnricher(element);
  m_collector->Collect(element);
  m_featureMaker->Add(element);
  FeatureBuilder feature;
  while (m_featureMaker->GetNextFeature(feature))
  {
    if (!m_filter->IsAccepted(feature))
      continue;

    m_collector->CollectFeature(feature, element);
    m_processor->Process(feature);
  }
}

void Translator::Finish()
{
  m_collector->Finish();
  m_processor->Finish();
}

bool Translator::Save()
{
  m_collector->Finalize(true /* isStable */);
  return true;
}
}  // namespace generator

#pragma once

#include "generator/collector_interface.hpp"
#include "generator/feature_maker_base.hpp"
#include "generator/filter_interface.hpp"
#include "generator/intermediate_data.hpp"
#include "generator/processor_interface.hpp"
#include "generator/relation_tags_enricher.hpp"
#include "generator/translator_interface.hpp"

#include <memory>
#include <string>
#include <vector>

struct OsmElement;

namespace generator
{
namespace cache
{
class IntermediateData;
}  // namespace cache

// Implementing this base class allows an object to create FeatureBuilder from OsmElement and then
// process it. You can add any collectors and filters.
class Translator : public TranslatorInterface
{
public:
  explicit Translator(std::shared_ptr<FeatureProcessorInterface> const & processor,
                      std::shared_ptr<cache::IntermediateData> const & cache,
                      std::shared_ptr<FeatureMakerBase> const & maker, std::shared_ptr<FilterInterface> const & filter,
                      std::shared_ptr<CollectorInterface> const & collector);
  explicit Translator(std::shared_ptr<FeatureProcessorInterface> const & processor,
                      std::shared_ptr<cache::IntermediateData> const & cache,
                      std::shared_ptr<FeatureMakerBase> const & maker);

  void SetCollector(std::shared_ptr<CollectorInterface> const & collector);
  void SetFilter(std::shared_ptr<FilterInterface> const & filter);

  // TranslatorInterface overrides:
  void Emit(OsmElement const & element) override;
  void Finish() override;
  bool Save() override;

protected:
  template <typename T>
  decltype(auto) CloneBase() const
  {
    auto cache = m_cache->Clone();
    auto processor = m_processor->Clone();
    auto featureMaker = m_featureMaker->Clone();
    auto filter = m_filter->Clone();
    auto collector = m_collector->Clone(cache->GetCache());
    return std::make_shared<T>(processor, cache, featureMaker, filter, collector);
  }

  void MergeIntoBase(Translator & other) const { other.m_collector->Merge(*m_collector); }

  std::shared_ptr<FilterInterface> m_filter;
  std::shared_ptr<CollectorInterface> m_collector;
  RelationTagsEnricher m_tagsEnricher;
  std::shared_ptr<FeatureMakerBase> m_featureMaker;
  std::shared_ptr<FeatureProcessorInterface> m_processor;
  std::shared_ptr<cache::IntermediateData> m_cache;
};
}  // namespace generator

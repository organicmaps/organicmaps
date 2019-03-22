#pragma once

#include "generator/collector_collection.hpp"
#include "generator/feature_maker_base.hpp"
#include "generator/filter_collection.hpp"
#include "generator/filter_interface.hpp"
#include "generator/relation_tags_enricher.hpp"
#include "generator/translator_interface.hpp"

#include <memory>
#include <string>
#include <vector>

struct OsmElement;

namespace generator
{
class EmitterInterface;
class CollectorInterface;

namespace cache
{
class IntermediateDataReader;
}  // namespace cache

// Implementing this base class allows an object to create FeatureBuilder1 from OsmElement and then process it.
// You can add any collectors and filters.
class Translator : public TranslatorInterface
{
public:
  explicit Translator(std::shared_ptr<EmitterInterface> emitter, cache::IntermediateDataReader & holder,
                      std::shared_ptr<FeatureMakerBase> maker, FilterCollection const & filters,
                      CollectorCollection const & collectors);
  explicit Translator(std::shared_ptr<EmitterInterface> emitter, cache::IntermediateDataReader & holder,
                      std::shared_ptr<FeatureMakerBase> maker);

  // TranslatorInterface overrides:
  void Emit(OsmElement & element) override;
  bool Finish() override;
  void GetNames(std::vector<std::string> & names) const override;

  void AddCollector(std::shared_ptr<CollectorInterface> collector);
  void AddCollectorCollection(CollectorCollection const & collectors);

  void AddFilter(std::shared_ptr<FilterInterface> filter);
  void AddFilterCollection(FilterCollection const & filters);

protected:
  FilterCollection m_filters;
  CollectorCollection m_collectors;
  RelationTagsEnricher m_tagsEnricher;
  std::shared_ptr<FeatureMakerBase> m_featureMaker;
  std::shared_ptr<EmitterInterface> m_emitter;
  cache::IntermediateDataReader & m_holder;
};
}  // namespace generator

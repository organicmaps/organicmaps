#pragma once

#include "generator/translator_interface.hpp"

#include "indexer/feature_data.hpp"

#include <memory>

struct OsmElement;
class FeatureBuilder1;
namespace feature
{
struct GenerateInfo;
}  // namespace feature

namespace generator
{
class EmitterInterface;
class RegionInfoCollector;
namespace cache
{
class IntermediateDataReader;
}  // namespace cache

// Osm to feature translator for regions.
class TranslatorRegion : public TranslatorInterface
{
public:
  explicit TranslatorRegion(std::shared_ptr<EmitterInterface> emitter,
                            cache::IntermediateDataReader & holder,
                            RegionInfoCollector & regionInfoCollector);

  void EmitElement(OsmElement * p) override;

private:
  bool IsSuitableElement(OsmElement const * p) const;
  void AddInfoAboutRegion(OsmElement const * p) const;
  bool ParseParams(OsmElement * p, FeatureParams & params) const;
  void BuildFeatureAndEmit(OsmElement const * p, FeatureParams & params);

private:
  std::shared_ptr<EmitterInterface> m_emitter;
  cache::IntermediateDataReader & m_holder;
  RegionInfoCollector & m_regionInfoCollector;
};
}  // namespace generator

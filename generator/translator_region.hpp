#pragma once

#include "generator/translator_interface.hpp"

#include "indexer/feature_data.hpp"

#include <memory>

class OsmElement;
class FeatureBuilder1;
namespace feature
{
class GenerateInfo;
}  // namespace feature

namespace generator
{
class EmitterInterface;
namespace cache
{
class IntermediateDataReader;
}  // namespace cache

// Osm to feature translator for regions.
class TranslatorRegion : public TranslatorInterface
{
public:
  TranslatorRegion(std::shared_ptr<EmitterInterface> emitter, cache::IntermediateDataReader & holder);

  void EmitElement(OsmElement * p) override;

private:
  bool IsSuitableElement(OsmElement const * p) const;
  void AddInfoAboutRegion(OsmElement const * p, FeatureBuilder1 & ft) const;
  bool ParseParams(OsmElement * p, FeatureParams & params) const;
  void BuildFeatureAndEmit(OsmElement const * p, FeatureParams & params);

private:
  std::shared_ptr<EmitterInterface> m_emitter;
  cache::IntermediateDataReader & m_holder;
};
}  // namespace generator

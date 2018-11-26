#pragma once

#include "generator/osm_element.hpp"
#include "generator/translator_interface.hpp"

#include "indexer/feature_data.hpp"

#include <memory>
#include <vector>
#include <string>

class FeatureBuilder1;
struct OsmElement;

namespace feature
{
struct GenerateInfo;
}  // namespace feature

namespace generator
{
class EmitterInterface;
namespace cache
{
class IntermediateDataReader;
}  // namespace cache

class CollectorInterface;
// TranslatorGeocoderBase class is responsible for processing only points and polygons.
class TranslatorGeocoderBase : public TranslatorInterface
{
public:
  explicit TranslatorGeocoderBase(std::shared_ptr<EmitterInterface> emitter,
                                  cache::IntermediateDataReader & holder);
  virtual ~TranslatorGeocoderBase() = default;

  // TranslatorInterface overrides:
  void EmitElement(OsmElement * p) override;
  bool Finish() override;
  void GetNames(std::vector<std::string> & names) const override;

  void AddCollector(std::shared_ptr<CollectorInterface> collector);

protected:
  virtual bool IsSuitableElement(OsmElement const * p) const = 0;

  bool ParseParams(OsmElement * p, FeatureParams & params) const;
  void BuildFeatureAndEmitFromRelation(OsmElement const * p, FeatureParams const & params);
  void BuildFeatureAndEmitFromWay(OsmElement const * p, FeatureParams const & params);
  void BuildFeatureAndEmitFromNode(OsmElement const * p, FeatureParams const & params);

private:
  void Emit(FeatureBuilder1 & fb, OsmElement const * p);

  std::shared_ptr<EmitterInterface> m_emitter;
  cache::IntermediateDataReader & m_holder;
  std::vector<std::shared_ptr<CollectorInterface>> m_collectors;
};
}  // namespace generator

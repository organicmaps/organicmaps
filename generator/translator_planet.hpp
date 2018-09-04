#pragma once

#include "generator/camera_info_collector.hpp"
#include "generator/metalines_builder.hpp"
#include "generator/relation_tags.hpp"
#include "generator/routing_helpers.hpp"
#include "generator/translator_interface.hpp"

#include "indexer/feature_data.hpp"

#include "base/geo_object_id.hpp"

#include <cstdint>
#include <functional>
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
namespace cache
{
class IntermediateDataReader;
}  // namespace cache

// Osm to feature translator for planet.
class TranslatorPlanet : public TranslatorInterface
{
public:
  TranslatorPlanet(std::shared_ptr<EmitterInterface> emitter, cache::IntermediateDataReader & holder,
                   feature::GenerateInfo const & info);

  // The main entry point for parsing process.
  void EmitElement(OsmElement * p) override;

private:
  bool ParseType(OsmElement * p, FeatureParams & params);
  void EmitFeatureBase(FeatureBuilder1 & ft, FeatureParams const & params) const;
  /// @param[in]  params  Pass by value because it can be modified.
  void EmitPoint(m2::PointD const & pt, FeatureParams params, base::GeoObjectId id) const;
  void EmitLine(FeatureBuilder1 & ft, FeatureParams params, bool isCoastLine) const;
  void EmitArea(FeatureBuilder1 & ft, FeatureParams params, std::function<void(FeatureBuilder1 &)> fn);

private:
  std::shared_ptr<EmitterInterface> m_emitter;
  cache::IntermediateDataReader & m_cache;
  uint32_t m_coastType;
  std::unique_ptr<FileWriter> m_addrWriter;
  routing::TagsProcessor m_routingTagsProcessor;
  RelationTagsNode m_nodeRelations;
  RelationTagsWay m_wayRelations;
  feature::MetalinesBuilder m_metalinesBuilder;
};
}  // namespace generator

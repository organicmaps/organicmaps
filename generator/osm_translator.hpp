#pragma once

#include "generator/feature_builder.hpp"
#include "generator/intermediate_data.hpp"
#include "generator/metalines_builder.hpp"
#include "generator/osm2type.hpp"
#include "generator/osm_element.hpp"
#include "generator/osm_source.hpp"
#include "generator/restriction_writer.hpp"
#include "generator/routing_helpers.hpp"
#include "generator/ways_merger.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature_visibility.hpp"
#include "indexer/ftypes_matcher.hpp"

#include "coding/file_writer.hpp"

#include "base/assert.hpp"
#include "base/cache.hpp"
#include "base/logging.hpp"
#include "base/stl_add.hpp"
#include "base/string_utils.hpp"
#include "base/osm_id.hpp"

#include <cstring>
#include <list>
#include <string>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <vector>

namespace generator {

/// Generated features should include parent relation tags to make
/// full types matching and storing any additional info.
class RelationTagsBase
{
public:
  explicit RelationTagsBase(routing::TagsProcessor & tagsProcessor);

  virtual ~RelationTagsBase() {}

  void Reset(uint64_t fID, OsmElement * p);

  template <class TReader>
  bool operator() (uint64_t id, TReader & reader)
  {
    bool exists = false;
    RelationElement & e = m_cache.Find(id, exists);
    if (!exists)
      CHECK(reader.Read(id, e), (id));

    Process(e);
    return false;
  }

protected:
  static bool IsSkipRelation(std::string const & type);
  bool IsKeyTagExists(std::string const & key) const;
  void AddCustomTag(std::pair<std::string, std::string> const & p);
  virtual void Process(RelationElement const & e) = 0;

protected:
  uint64_t m_featureID;
  OsmElement * m_current;
  routing::TagsProcessor & m_routingTagsProcessor;

private:
  my::Cache<uint64_t, RelationElement> m_cache;
};

class RelationTagsNode : public RelationTagsBase
{
  using TBase = RelationTagsBase;

public:
  explicit RelationTagsNode(routing::TagsProcessor & tagsProcessor);

protected:
  void Process(RelationElement const & e) override;
};


class RelationTagsWay : public RelationTagsBase
{
public:
  explicit RelationTagsWay(routing::TagsProcessor & routingTagsProcessor);

private:
  using TBase = RelationTagsBase;
  using TNameKeys = std::unordered_set<std::string>;

  bool IsAcceptBoundary(RelationElement const & e) const;

protected:
  void Process(RelationElement const & e) override;
};


class HolesAccumulator
{
public:
  explicit HolesAccumulator(cache::IntermediateDataReader & holder);

  void operator() (uint64_t id) { m_merger.AddWay(id); }
  FeatureBuilder1::Geometry & GetHoles();

private:
  AreaWayMerger m_merger;
  FeatureBuilder1::Geometry m_holes;
};


/// Find holes for way with 'id' in first relation.
class HolesProcessor
{
public:
  HolesProcessor(uint64_t id, cache::IntermediateDataReader & holder);

  /// 1. relations process function
  bool operator() (uint64_t /*id*/, RelationElement const & e);
  /// 2. "ways in relation" process function
  void operator() (uint64_t id, std::string const & role);
  FeatureBuilder1::Geometry & GetHoles() { return m_holes.GetHoles(); }

private:
  uint64_t m_id;      ///< id of way to find it's holes
  HolesAccumulator m_holes;
};


class OsmToFeatureTranslatorInterface
{
public:
  virtual ~OsmToFeatureTranslatorInterface() {}

  virtual void EmitElement(OsmElement * p) = 0;
};


class OsmToFeatureTranslator : public OsmToFeatureTranslatorInterface
{
public:
  OsmToFeatureTranslator(std::shared_ptr<EmitterInterface> emitter,
                         cache::IntermediateDataReader & holder,
                         feature::GenerateInfo const & info);

  /// The main entry point for parsing process.
  void EmitElement(OsmElement * p) override;

private:
  bool ParseType(OsmElement * p, FeatureParams & params);
  void EmitFeatureBase(FeatureBuilder1 & ft, FeatureParams const & params) const;
  /// @param[in]  params  Pass by value because it can be modified.
  void EmitPoint(m2::PointD const & pt, FeatureParams params, osm::Id id) const;
  void EmitLine(FeatureBuilder1 & ft, FeatureParams params, bool isCoastLine) const;

  template <class MakeFnT>
  void EmitArea(FeatureBuilder1 & ft, FeatureParams params, MakeFnT makeFn)
  {
    using namespace feature;

    // Ensure that we have closed area geometry.
    if (!ft.IsGeometryClosed())
      return;

    if (ftypes::IsTownOrCity(params.m_types))
    {
      auto fb = ft;
      makeFn(fb);
      m_emitter->EmitCityBoundary(fb, params);
    }

    // Key point here is that IsDrawableLike and RemoveNoDrawableTypes
    // work a bit different for GEOM_AREA.
    if (IsDrawableLike(params.m_types, GEOM_AREA))
    {
      // Make the area feature if it has unique area styles.
      VERIFY(RemoveNoDrawableTypes(params.m_types, GEOM_AREA), (params));

      makeFn(ft);

      EmitFeatureBase(ft, params);
    }
    else
    {
      // Try to make the point feature if it has point styles.
      EmitPoint(ft.GetGeometryCenter(), params, ft.GetLastOsmId());
    }
  }

private:
  std::shared_ptr<EmitterInterface> m_emitter;
  cache::IntermediateDataReader & m_holder;
  uint32_t m_coastType;
  std::unique_ptr<FileWriter> m_addrWriter;

  routing::TagsProcessor m_routingTagsProcessor;

  RelationTagsNode m_nodeRelations;
  RelationTagsWay m_wayRelations;
  feature::MetalinesBuilder m_metalinesBuilder;
};

class OsmToFeatureTranslatorRegion : public OsmToFeatureTranslatorInterface
{
public:
  OsmToFeatureTranslatorRegion(std::shared_ptr<EmitterInterface> emitter,
                               cache::IntermediateDataReader & holder);

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

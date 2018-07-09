#pragma once

#include "generator/feature_builder.hpp"
#include "generator/metalines_builder.hpp"
#include "generator/osm2type.hpp"
#include "generator/osm_element.hpp"
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

#include <list>
#include <type_traits>
#include <unordered_set>
#include <vector>

namespace
{
/// Generated features should include parent relation tags to make
/// full types matching and storing any additional info.
class RelationTagsBase
{
public:
  RelationTagsBase(routing::TagsProcessor & tagsProcessor)
    : m_routingTagsProcessor(tagsProcessor), m_cache(14)
  {
  }

  void Reset(uint64_t fID, OsmElement * p)
  {
    m_featureID = fID;
    m_current = p;
  }

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
  static bool IsSkipRelation(std::string const & type)
  {
    /// @todo Skip special relation types.
    return (type == "multipolygon" || type == "bridge");
  }

  bool IsKeyTagExists(std::string const & key) const
  {
    for (auto const & p : m_current->m_tags)
      if (p.key == key)
        return true;
    return false;
  }

  void AddCustomTag(pair<std::string, std::string> const & p)
  {
    m_current->AddTag(p.first, p.second);
  }

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
  RelationTagsNode(routing::TagsProcessor & tagsProcessor) : RelationTagsBase(tagsProcessor) {}

protected:
  void Process(RelationElement const & e) override
  {
    std::string const & type = e.GetType();
    if (TBase::IsSkipRelation(type))
      return;

    if (type == "restriction")
    {
      m_routingTagsProcessor.m_restrictionWriter.Write(e);
      return;
    }

    bool const processAssociatedStreet = type == "associatedStreet" &&
        TBase::IsKeyTagExists("addr:housenumber") && !TBase::IsKeyTagExists("addr:street");

    for (auto const & p : e.tags)
    {
      // - used in railway station processing
      // - used in routing information
      // - used in building addresses matching
      if (p.first == "network" || p.first == "operator" || p.first == "route" ||
          p.first == "maxspeed" ||
          strings::StartsWith(p.first, "addr:"))
      {
        if (!TBase::IsKeyTagExists(p.first))
          TBase::AddCustomTag(p);
      }
      // Convert associatedStreet relation name to addr:street tag if we don't have one.
      else if (p.first == "name" && processAssociatedStreet)
        TBase::AddCustomTag({"addr:street", p.second});
    }
  }
};

class RelationTagsWay : public RelationTagsBase
{
public:
  RelationTagsWay(routing::TagsProcessor & routingTagsProcessor)
    : RelationTagsBase(routingTagsProcessor)
  {
  }

private:
  using TBase = RelationTagsBase;
  using TNameKeys = std::unordered_set<std::string>;

  bool IsAcceptBoundary(RelationElement const & e) const
  {
    std::string role;
    CHECK(e.FindWay(TBase::m_featureID, role), (TBase::m_featureID));

    // Do not accumulate boundary types (boundary=administrative) for inner polygons.
    // Example: Minsk city border (admin_level=8) is inner for Minsk area border (admin_level=4).
    return (role != "inner");
  }

protected:
  void Process(RelationElement const & e) override
  {
    /// @todo Review route relations in future.
    /// Actually, now they give a lot of dummy tags.
    std::string const & type = e.GetType();
    if (TBase::IsSkipRelation(type))
      return;

    if (type == "route")
    {
      if (e.GetTagValue("route") == "road")
      {
        // Append "network/ref" to the feature ref tag.
        std::string ref = e.GetTagValue("ref");
        if (!ref.empty())
        {
          std::string const & network = e.GetTagValue("network");
          // Not processing networks with more than 15 chars (see road_shields_parser.cpp).
          if (!network.empty() && network.find('/') == std::string::npos && network.size() < 15)
            ref = network + '/' + ref;
          std::string const & refBase = m_current->GetTag("ref");
          if (!refBase.empty())
            ref = refBase + ';' + ref;
          TBase::AddCustomTag({"ref", std::move(ref)});
        }
      }
      return;
    }

    if (type == "restriction")
    {
      m_routingTagsProcessor.m_restrictionWriter.Write(e);
      return;
    }

    if (type == "building")
    {
      // If this way has "outline" role, add [building=has_parts] type.
      if (e.GetWayRole(m_current->id) == "outline")
        TBase::AddCustomTag({"building", "has_parts"});
      return;
    }

    bool const isBoundary = (type == "boundary") && IsAcceptBoundary(e);
    bool const processAssociatedStreet = type == "associatedStreet" &&
        TBase::IsKeyTagExists("addr:housenumber") && !TBase::IsKeyTagExists("addr:street");
    bool const isHighway = TBase::IsKeyTagExists("highway");

    for (auto const & p : e.tags)
    {
      /// @todo Skip common key tags.
      if (p.first == "type" || p.first == "route" || p.first == "area")
        continue;

      // Convert associatedStreet relation name to addr:street tag if we don't have one.
      if (p.first == "name" && processAssociatedStreet)
        TBase::AddCustomTag({"addr:street", p.second});

      // Important! Skip all "name" tags.
      if (strings::StartsWith(p.first, "name") || p.first == "int_name")
        continue;

      if (!isBoundary && p.first == "boundary")
        continue;

      if (p.first == "place")
        continue;

      // Do not pass "ref" tags from boundaries and other, non-route relations to highways.
      if (p.first == "ref" && isHighway)
        continue;

      TBase::AddCustomTag(p);
    }
  }
};
}  // namespace

/// @param  TEmitter  Feature accumulating policy
/// @param  TCache   Nodes, ways, relations holder
template <class TEmitter, class TCache>
class OsmToFeatureTranslator
{
  TEmitter & m_emitter;
  TCache & m_holder;
  uint32_t m_coastType;
  unique_ptr<FileWriter> m_addrWriter;

  routing::TagsProcessor m_routingTagsProcessor;

  RelationTagsNode m_nodeRelations;
  RelationTagsWay m_wayRelations;
  feature::MetalinesBuilder m_metalinesBuilder;

  class HolesAccumulator
  {
    AreaWayMerger<TCache> m_merger;
    FeatureBuilder1::TGeometry m_holes;

  public:
    HolesAccumulator(OsmToFeatureTranslator * pMain) : m_merger(pMain->m_holder) {}

    void operator() (uint64_t id) { m_merger.AddWay(id); }

    FeatureBuilder1::TGeometry & GetHoles()
    {
      ASSERT(m_holes.empty(), ("Can call only once"));
      m_merger.ForEachArea(false, [this](FeatureBuilder1::TPointSeq & v, std::vector<uint64_t> const &)
      {
        m_holes.push_back(FeatureBuilder1::TPointSeq());
        m_holes.back().swap(v);
      });
      return m_holes;
    }
  };

  /// Find holes for way with 'id' in first relation.
  class HolesProcessor
  {
    uint64_t m_id;      ///< id of way to find it's holes
    HolesAccumulator m_holes;

  public:
    HolesProcessor(uint64_t id, OsmToFeatureTranslator * pMain) : m_id(id), m_holes(pMain) {}

    /// 1. relations process function
    bool operator() (uint64_t /*id*/, RelationElement const & e)
    {
      if (e.GetType() != "multipolygon")
        return false;
      std::string role;
      if (e.FindWay(m_id, role) && (role == "outer"))
      {
        e.ForEachWay(*this);
        // stop processing (??? assume that "outer way" exists in one relation only ???)
        return true;
      }
      return false;
    }

    /// 2. "ways in relation" process function
    void operator() (uint64_t id, std::string const & role)
    {
      if (id != m_id && role == "inner")
        m_holes(id);
    }

    FeatureBuilder1::TGeometry & GetHoles() { return m_holes.GetHoles(); }
  };

  bool ParseType(OsmElement * p, FeatureParams & params)
  {
    // Get tags from parent relations.
    if (p->type == OsmElement::EntityType::Node)
    {
      m_nodeRelations.Reset(p->id, p);
      m_holder.ForEachRelationByNodeCached(p->id, m_nodeRelations);
    }
    else if (p->type == OsmElement::EntityType::Way)
    {
      m_wayRelations.Reset(p->id, p);
      m_holder.ForEachRelationByWayCached(p->id, m_wayRelations);
    }

    // Get params from element tags.
    ftype::GetNameAndType(p, params);
    if (!params.IsValid())
      return false;

    m_routingTagsProcessor.m_roadAccessWriter.Process(*p);
    return true;
  }

  void EmitFeatureBase(FeatureBuilder1 & ft, FeatureParams const & params) const
  {
    ft.SetParams(params);
    if (ft.PreSerialize())
    {
      std::string addr;
      if (m_addrWriter && ftypes::IsBuildingChecker::Instance()(params.m_Types) &&
          ft.FormatFullAddress(addr))
      {
        m_addrWriter->Write(addr.c_str(), addr.size());
      }

      m_emitter(ft);
    }
  }

  /// @param[in]  params  Pass by value because it can be modified.
  //@{
  void EmitPoint(m2::PointD const & pt, FeatureParams params, osm::Id id) const
  {
    if (feature::RemoveNoDrawableTypes(params.m_Types, feature::GEOM_POINT))
    {
      FeatureBuilder1 ft;
      ft.SetCenter(pt);
      ft.SetOsmId(id);
      EmitFeatureBase(ft, params);
    }
  }

  void EmitLine(FeatureBuilder1 & ft, FeatureParams params, bool isCoastLine) const
  {
    if (isCoastLine || feature::RemoveNoDrawableTypes(params.m_Types, feature::GEOM_LINE))
    {
      ft.SetLinear(params.m_reverseGeometry);
      EmitFeatureBase(ft, params);
    }
  }

  template <class MakeFnT>
  void EmitArea(FeatureBuilder1 & ft, FeatureParams params, MakeFnT makeFn)
  {
    using namespace feature;

    // Ensure that we have closed area geometry.
    if (!ft.IsGeometryClosed())
      return;

    if (ftypes::IsTownOrCity(params.m_Types))
    {
      auto fb = ft;
      makeFn(fb);
      m_emitter.EmitCityBoundary(fb, params);
    }

    // Key point here is that IsDrawableLike and RemoveNoDrawableTypes
    // work a bit different for GEOM_AREA.
    if (IsDrawableLike(params.m_Types, GEOM_AREA))
    {
      // Make the area feature if it has unique area styles.
      VERIFY(RemoveNoDrawableTypes(params.m_Types, GEOM_AREA), (params));

      makeFn(ft);

      EmitFeatureBase(ft, params);
    }
    else
    {
      // Try to make the point feature if it has point styles.
      EmitPoint(ft.GetGeometryCenter(), params, ft.GetLastOsmId());
    }
  }
  //@}

public:
  /// The main entry point for parsing process.
  void EmitElement(OsmElement * p)
  {
    enum class FeatureState {Unknown, Ok, EmptyTags, HasNoTypes, BrokenRef, ShortGeom, InvalidType};

    CHECK(p, ("Tried to emit a null OsmElement"));

    FeatureParams params;
    FeatureState state = FeatureState::Unknown;

    switch(p->type)
    {
      case OsmElement::EntityType::Node:
      {
        if (p->m_tags.empty())
        {
          state = FeatureState::EmptyTags;
          break;
        }

        if (!ParseType(p, params))
        {
          state = FeatureState::HasNoTypes;
          break;
        }

        m2::PointD const pt = MercatorBounds::FromLatLon(p->lat, p->lon);
        EmitPoint(pt, params, osm::Id::Node(p->id));
        state = FeatureState::Ok;
        break;
      }

      case OsmElement::EntityType::Way:
      {
        FeatureBuilder1 ft;

        // Parse geometry.
        for (uint64_t ref : p->Nodes())
        {
          m2::PointD pt;
          if (!m_holder.GetNode(ref, pt.y, pt.x))
          {
            state = FeatureState::BrokenRef;
            break;
          }
          ft.AddPoint(pt);
        }

        if (state == FeatureState::BrokenRef)
          break;

        if (ft.GetPointsCount() < 2)
        {
          state = FeatureState::ShortGeom;
          break;
        }

        if (!ParseType(p, params))
        {
          state = FeatureState::HasNoTypes;
          break;
        }

        ft.SetOsmId(osm::Id::Way(p->id));
        bool isCoastLine = (m_coastType != 0 && params.IsTypeExist(m_coastType));

        EmitArea(ft, params, [&] (FeatureBuilder1 & ft)
        {
          isCoastLine = false;  // emit coastline feature only once
          HolesProcessor processor(p->id, this);
          m_holder.ForEachRelationByWay(p->id, processor);
          ft.SetAreaAddHoles(processor.GetHoles());
        });

        m_metalinesBuilder(*p, params);
        EmitLine(ft, params, isCoastLine);
        state = FeatureState::Ok;
        break;
      }

      case OsmElement::EntityType::Relation:
      {
        {
          // 1. Check, if this is our processable relation. Here we process only polygon relations.
          size_t i = 0;
          size_t const count = p->m_tags.size();
          for (; i < count; ++i)
          {
            if (p->m_tags[i].key == "type" && p->m_tags[i].value == "multipolygon")
              break;
          }
          if (i == count)
          {
            state = FeatureState::InvalidType;
            break;
          }
        }

        if (!ParseType(p, params))
        {
          state = FeatureState::HasNoTypes;
          break;
        }

        HolesAccumulator holes(this);
        AreaWayMerger<TCache> outer(m_holder);

        // 3. Iterate ways to get 'outer' and 'inner' geometries
        for (auto const & e : p->Members())
        {
          if (e.type != OsmElement::EntityType::Way)
            continue;

          if (e.role == "outer")
            outer.AddWay(e.ref);
          else if (e.role == "inner")
            holes(e.ref);
        }

        auto const & holesGeometry = holes.GetHoles();
        outer.ForEachArea(true, [&] (FeatureBuilder1::TPointSeq const & pts, std::vector<uint64_t> const & ids)
        {
          FeatureBuilder1 ft;

          for (uint64_t id : ids)
            ft.AddOsmId(osm::Id::Way(id));

          for (auto const & pt : pts)
            ft.AddPoint(pt);

          ft.AddOsmId(osm::Id::Relation(p->id));
          EmitArea(ft, params, [&holesGeometry] (FeatureBuilder1 & ft) {ft.SetAreaAddHoles(holesGeometry);});
        });

        state = FeatureState::Ok;
        break;
      }

      default:
        state = FeatureState::Unknown;
        break;
    }

//    if (state == FeatureState::Ok)
//    {
//      Classificator const & c = classif();
//      static char const * const stateText[] = {"U", "Ok", "ET", "HNT", "BR", "SG", "IT"};
//      stringstream ss;
//      ss << p->id << " [" << stateText[static_cast<typename underlying_type<FeatureState>::type>(state)] << "]";
//      for (auto const & p : params.m_Types)
//        ss << " " << c.GetReadableObjectName(p);
//      ss << endl;
//      std::ofstream file("feature_types_new2.txt", ios::app);
//      file.write(ss.str().data(), ss.str().size());
//    }
  }

public:
  OsmToFeatureTranslator(TEmitter & emitter, TCache & holder, uint32_t coastType,
                         std::string const & addrFilePath = {},
                         std::string const & restrictionsFilePath = {},
                         std::string const & roadAccessFilePath = {},
                         std::string const & metalinesFilePath = {})
    : m_emitter(emitter)
    , m_holder(holder)
    , m_coastType(coastType)
    , m_nodeRelations(m_routingTagsProcessor)
    , m_wayRelations(m_routingTagsProcessor)
    , m_metalinesBuilder(metalinesFilePath)
  {
    if (!addrFilePath.empty())
      m_addrWriter.reset(new FileWriter(addrFilePath));

    if (!restrictionsFilePath.empty())
      m_routingTagsProcessor.m_restrictionWriter.Open(restrictionsFilePath);

    if (!roadAccessFilePath.empty())
      m_routingTagsProcessor.m_roadAccessWriter.Open(roadAccessFilePath);
  }
};

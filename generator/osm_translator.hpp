#pragma once

#include "generator/feature_builder.hpp"
#include "generator/osm2type.hpp"
#include "generator/osm_element.hpp"
#include "generator/restriction_writer.hpp"
#include "generator/ways_merger.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature_visibility.hpp"
#include "indexer/ftypes_matcher.hpp"

#include "coding/file_writer.hpp"

#include "base/cache.hpp"
#include "base/logging.hpp"
#include "base/stl_add.hpp"
#include "base/string_utils.hpp"

#include "std/list.hpp"
#include "std/type_traits.hpp"
#include "std/unordered_set.hpp"
#include "std/vector.hpp"

namespace
{
/// Generated features should include parent relation tags to make
/// full types matching and storing any additional info.
class RelationTagsBase
{
public:
  RelationTagsBase(routing::RestrictionWriter & restrictionWriter)
    : m_restrictionWriter(restrictionWriter), m_cache(14)
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
  static bool IsSkipRelation(string const & type)
  {
    /// @todo Skip special relation types.
    return (type == "multipolygon" || type == "bridge");
  }

  bool IsKeyTagExists(string const & key) const
  {
    for (auto const & p : m_current->m_tags)
      if (p.key == key)
        return true;
    return false;
  }

  void AddCustomTag(pair<string, string> const & p)
  {
    m_current->AddTag(p.first, p.second);
  }

  virtual void Process(RelationElement const & e) = 0;

protected:
  uint64_t m_featureID;
  OsmElement * m_current;
  routing::RestrictionWriter & m_restrictionWriter;

private:
  my::Cache<uint64_t, RelationElement> m_cache;
};

class RelationTagsNode : public RelationTagsBase
{
  using TBase = RelationTagsBase;

public:
  RelationTagsNode(routing::RestrictionWriter & restrictionWriter)
    : RelationTagsBase(restrictionWriter)
  {
  }

protected:
  void Process(RelationElement const & e) override
  {
    string const & type = e.GetType();
    if (TBase::IsSkipRelation(type))
      return;

    if (type == "restriction")
    {
      m_restrictionWriter.Write(e);
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
  RelationTagsWay(routing::RestrictionWriter & restrictionWriter)
    : RelationTagsBase(restrictionWriter)
  {
  }

private:
  using TBase = RelationTagsBase;
  using TNameKeys = unordered_set<string>;

  bool IsAcceptBoundary(RelationElement const & e) const
  {
    string role;
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
    string const & type = e.GetType();
    if (TBase::IsSkipRelation(type) || type == "route")
      return;

    if (type == "restriction")
    {
      m_restrictionWriter.Write(e);
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

  routing::RestrictionWriter m_restrictionWriter;

  RelationTagsNode m_nodeRelations;
  RelationTagsWay m_wayRelations;

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
      m_merger.ForEachArea(false, [this](FeatureBuilder1::TPointSeq & v, vector<uint64_t> const &)
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
      string role;
      if (e.FindWay(m_id, role) && (role == "outer"))
      {
        e.ForEachWay(*this);
        // stop processing (??? assume that "outer way" exists in one relation only ???)
        return true;
      }
      return false;
    }

    /// 2. "ways in relation" process function
    void operator() (uint64_t id, string const & role)
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
    return params.IsValid();
  }

  void EmitFeatureBase(FeatureBuilder1 & ft, FeatureParams const & params) const
  {
    ft.SetParams(params);
    if (ft.PreSerialize())
    {
      string addr;
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
  void EmitArea(FeatureBuilder1 & ft, FeatureParams params, MakeFnT makeFn) const
  {
    using namespace feature;

    // Ensure that we have closed area geometry.
    if (!ft.IsGeometryClosed())
      return;

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
        outer.ForEachArea(true, [&] (FeatureBuilder1::TPointSeq const & pts, vector<uint64_t> const & ids)
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
                         string const & addrFilePath = {}, string const & restrictionsFilePath = {})
    : m_emitter(emitter)
    , m_holder(holder)
    , m_coastType(coastType)
    , m_nodeRelations(m_restrictionWriter)
    , m_wayRelations(m_restrictionWriter)
  {
    if (!addrFilePath.empty())
      m_addrWriter.reset(new FileWriter(addrFilePath));

    if (!restrictionsFilePath.empty())
      m_restrictionWriter.Open(restrictionsFilePath);
  }
};

#pragma once

#include "osm2type.hpp"
#include "xml_element.hpp"
#include "feature_builder.hpp"
#include "ways_merger.hpp"

#include "../indexer/ftypes_matcher.hpp"
#include "../indexer/feature_visibility.hpp"

#include "../base/string_utils.hpp"
#include "../base/logging.hpp"
#include "../base/stl_add.hpp"
#include "../base/cache.hpp"

#include "../std/unordered_map.hpp"
#include "../std/list.hpp"


/// @param  TEmitter  Feature accumulating policy
/// @param  THolder   Nodes, ways, relations holder
template <class TEmitter, class THolder>
class SecondPassParser : public BaseOSMParser
{
  TEmitter & m_emitter;
  THolder & m_holder;


  bool GetPoint(uint64_t id, m2::PointD & pt) const
  {
    return m_holder.GetNode(id, pt.y, pt.x);
  }

  typedef vector<m2::PointD> pts_vec_t;
  typedef list<pts_vec_t> holes_list_t;

  class holes_accumulator
  {
    AreaWayMerger<THolder> m_merger;
    holes_list_t m_holes;

  public:
    holes_accumulator(SecondPassParser * pMain) : m_merger(pMain->m_holder) {}

    void operator() (uint64_t id)
    {
      m_merger.AddWay(id);
    }

    void operator() (pts_vec_t & v, vector<uint64_t> const &)
    {
      m_holes.push_back(pts_vec_t());
      m_holes.back().swap(v);
    }

    holes_list_t & GetHoles()
    {
      ASSERT ( m_holes.empty(), ("Can call only once") );
      m_merger.ForEachArea(*this, false);
      return m_holes;
    }
  };

  /// Find holes for way with 'id' in first relation.
  class multipolygon_holes_processor
  {
    uint64_t m_id;      ///< id of way to find it's holes
    holes_accumulator m_holes;

  public:
    multipolygon_holes_processor(uint64_t id, SecondPassParser * pMain)
      : m_id(id), m_holes(pMain)
    {
    }

    /// 1. relations process function
    bool operator() (uint64_t /*id*/, RelationElement const & e)
    {
      if (e.GetType() == "multipolygon")
      {
        string role;
        if (e.FindWay(m_id, role) && (role == "outer"))
        {
          e.ForEachWay(*this);
          // stop processing (??? assume that "outer way" exists in one relation only ???)
          return true;
        }
      }
      return false;
    }

    /// 2. "ways in relation" process function
    void operator() (uint64_t id, string const & role)
    {
      if (id != m_id && role == "inner")
        m_holes(id);
    }

    holes_list_t & GetHoles() { return m_holes.GetHoles(); }
  };

  /// Generated features should include parent relation tags to make
  /// full types matching and storing any additional info.
  class RelationTagsProcessor
  {
    uint64_t m_featureID;
    XMLElement * m_current;

    my::Cache<uint64_t, RelationElement> m_cache;

    bool IsAcceptBoundary(RelationElement const & e) const
    {
      string role;
      CHECK(e.FindWay(m_featureID, role), (m_featureID));

      // Do not accumulate boundary types (boundary=administrative) for inner polygons.
      // Example: Minsk city border (admin_level=8) is inner for Minsk area border (admin_level=4).
      return (role != "inner");
    }

    bool HasName() const
    {
      for (auto p : m_current->childs)
      {
        if (p.name == "tag")
          for (auto a : p.attrs)
            if (strings::StartsWith(a.first, "name"))
              return true;
      }

      return false;
    }

    void Process(RelationElement const & e)
    {
      string const type = e.GetType();

      /// @todo Skip special relation types.
      if (type == "multipolygon" ||
          type == "route" ||
          type == "bridge" ||
          type == "restriction")
      {
        return;
      }

      bool const isWay = (m_current->name == "way");
      bool const isBoundary = isWay && (type == "boundary") && IsAcceptBoundary(e);
      bool const hasName = HasName();

      for (auto p : e.tags)
      {
        /// @todo Skip common key tags.
        if (p.first == "type" || p.first == "route")
          continue;

        if (hasName && strings::StartsWith(p.first, "name"))
          continue;

        if (!isBoundary && p.first == "boundary")
          continue;

        if (isWay && p.first == "place")
          continue;

        m_current->AddKV(p.first, p.second);
      }
    }

  public:
    RelationTagsProcessor()
      : m_cache(14)
    {
    }

    void Reset(uint64_t fID, XMLElement * p)
    {
      m_featureID = fID;
      m_current = p;
    }

    template <class ReaderT> bool operator() (uint64_t id, ReaderT & reader)
    {
      bool exists = false;
      RelationElement & e = m_cache.Find(id, exists);
      if (!exists)
        CHECK(reader.Read(id, e), (id));

      Process(e);
      return false;
    }

  } m_relationsProcess;

  bool ParseType(XMLElement * p, uint64_t & id, FeatureParams & params)
  {
    CHECK(strings::to_uint64(p->attrs["id"], id), (p->attrs["id"]));

    // Get tags from parent relations.
    m_relationsProcess.Reset(id, p);

    if (p->name == "node")
    {
      // additional process of nodes ONLY if there is no native types
      FeatureParams fp;
      ftype::GetNameAndType(p, fp);
      if (!ftype::IsValidTypes(fp))
        m_holder.ForEachRelationByNodeCached(id, m_relationsProcess);
    }
    else if (p->name == "way")
    {
      // always make additional process of ways
      m_holder.ForEachRelationByWayCached(id, m_relationsProcess);
    }

    // Get params from element tags.
    ftype::GetNameAndType(p, params);
    params.FinishAddingTypes();
    return ftype::IsValidTypes(params);
  }

  typedef FeatureBuilder1 FeatureBuilderT;

  class multipolygons_emitter
  {
    SecondPassParser * m_pMain;
    FeatureParams const & m_params;
    holes_list_t & m_holes;
    uint64_t m_relID;

  public:
    multipolygons_emitter(SecondPassParser * pMain,
                          FeatureParams const & params,
                          holes_list_t & holes,
                          uint64_t relID)
      : m_pMain(pMain), m_params(params), m_holes(holes), m_relID(relID)
    {
    }

    void operator() (pts_vec_t const & pts, vector<uint64_t> const & ids)
    {
      FeatureBuilderT ft;

      for (size_t i = 0; i < ids.size(); ++i)
        ft.AddOsmId(osm::Id::Way(ids[i]));

      for (size_t i = 0; i < pts.size(); ++i)
        ft.AddPoint(pts[i]);

      ft.AddOsmId(osm::Id::Relation(m_relID));
      m_pMain->EmitArea(ft, m_params, [this] (FeatureBuilderT & ft)
      {
        ft.SetAreaAddHoles(m_holes);
      });
    }
  };

  uint32_t m_coastType;

  unique_ptr<FileWriter> m_addrWriter;
  bool NeedWriteAddress(FeatureParams const & params) const
  {
    static ftypes::IsBuildingChecker const checker;
    return m_addrWriter && checker(params.m_Types);
  }

  void EmitFeatureBase(FeatureBuilderT & ft, FeatureParams const & params)
  {
    ft.SetParams(params);
    if (ft.PreSerialize())
    {
      string addr;
      if (NeedWriteAddress(params) && ft.FormatFullAddress(addr))
        m_addrWriter->Write(addr.c_str(), addr.size());

      m_emitter(ft);
    }
  }

  /// @param[in]  params  Pass by value because it can be modified.
  //@{
  void EmitPoint(m2::PointD const & pt, FeatureParams params, osm::Id id)
  {
    if (feature::RemoveNoDrawableTypes(params.m_Types, feature::GEOM_POINT))
    {
      FeatureBuilderT ft;
      ft.SetCenter(pt);
      ft.SetOsmId(id);
      EmitFeatureBase(ft, params);
    }
  }

  void EmitLine(FeatureBuilderT & ft, FeatureParams params, bool isCoastLine)
  {
    if (isCoastLine || feature::RemoveNoDrawableTypes(params.m_Types, feature::GEOM_LINE))
    {
      ft.SetLinear(params.m_reverseGeometry);
      EmitFeatureBase(ft, params);
    }
  }

  template <class MakeFnT>
  void EmitArea(FeatureBuilderT & ft, FeatureParams params, MakeFnT makeFn)
  {
    using namespace feature;

    // Ensure that we have closed area geometry.
    if (ft.IsGeometryClosed())
    {
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
  }
  //@}

  /// The main entry point for parsing process.
  virtual void EmitElement(XMLElement * p)
  {
    uint64_t id;
    FeatureParams params;
    if (!ParseType(p, id, params))
      return;

    if (p->name == "node")
    {
      m2::PointD pt;
      if (p->childs.empty() || !GetPoint(id, pt))
        return;

      EmitPoint(pt, params, osm::Id::Node(id));
    }
    else if (p->name == "way")
    {
      FeatureBuilderT ft;

      // Parse geometry.
      for (size_t i = 0; i < p->childs.size(); ++i)
      {
        if (p->childs[i].name == "nd")
        {
          uint64_t nodeID;
          CHECK ( strings::to_uint64(p->childs[i].attrs["ref"], nodeID), (p->childs[i].attrs["ref"]) );

          m2::PointD pt;
          if (!GetPoint(nodeID, pt))
            return;

          ft.AddPoint(pt);
        }
      }

      if (ft.GetPointsCount() < 2)
        return;

      ft.SetOsmId(osm::Id::Way(id));
      bool isCoastLine = (m_coastType != 0 && params.IsTypeExist(m_coastType));

      EmitArea(ft, params, [&] (FeatureBuilderT & ft)
      {
        isCoastLine = false;  // emit coastline feature only once
        multipolygon_holes_processor processor(id, this);
        m_holder.ForEachRelationByWay(id, processor);
        ft.SetAreaAddHoles(processor.GetHoles());
      });

      EmitLine(ft, params, isCoastLine);
    }
    else if (p->name == "relation")
    {
      {
        // 1. Check, if this is our processable relation. Here we process only polygon relations.
        size_t i = 0;
        size_t const count = p->childs.size();
        for (; i < count; ++i)
        {
          if (p->childs[i].name == "tag" &&
              p->childs[i].attrs["k"] == "type" &&
              p->childs[i].attrs["v"] == "multipolygon")
          {
            break;
          }
        }
        if (i == count)
          return;
      }

      holes_accumulator holes(this);
      AreaWayMerger<THolder> outer(m_holder);

      // 3. Iterate ways to get 'outer' and 'inner' geometries
      for (size_t i = 0; i < p->childs.size(); ++i)
      {
        if (p->childs[i].name == "member" &&
            p->childs[i].attrs["type"] == "way")
        {
          string const & role = p->childs[i].attrs["role"];
          uint64_t wayID;
          CHECK ( strings::to_uint64(p->childs[i].attrs["ref"], wayID), (p->childs[i].attrs["ref"]) );

          if (role == "outer")
            outer.AddWay(wayID);
          else if (role == "inner")
            holes(wayID);
        }
      }

      multipolygons_emitter emitter(this, params, holes.GetHoles(), id);
      outer.ForEachArea(emitter, true);
    }
  }

public:
  SecondPassParser(TEmitter & emitter, THolder & holder,
                   uint32_t coastType, string const & addrFilePath)
    : m_emitter(emitter), m_holder(holder), m_coastType(coastType)
  {
    char const * tags[] = { "osm", "node", "way", "relation" };
    SetTags(tags);

    if (!addrFilePath.empty())
      m_addrWriter.reset(new FileWriter(addrFilePath));
  }
};

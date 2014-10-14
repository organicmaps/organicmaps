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

  /// Feature types processor.
  class type_processor
  {
    static void MakeXMLElement(RelationElement const & rel, XMLElement & out)
    {
      for (auto i = rel.tags.begin(); i != rel.tags.end(); ++i)
        if (i->first != "type")
          out.AddKV(i->first, i->second);
    }

    /// @param[in]  ID of processing feature.
    uint64_t m_featureID;

    /// @param[out] Feature value as result.
    FeatureParams * m_val;

    /// Cache: relation id -> feature value (for fast feature parsing)
    struct RelationValue
    {
      FeatureParams m_p;
      RelationElement * m_e;

      RelationValue() : m_e(0) {}
    };

    typedef unordered_map<uint64_t, RelationValue> RelationCacheT;
    RelationCacheT m_typeCache;

    bool IsAcceptBoundaryTypes(RelationElement const & rel) const
    {
      string role;
      if (!rel.FindWay(m_featureID, role))
      {
        // This case is possible when we found the relation by node (just skip it).
        CHECK ( rel.FindNode(m_featureID, role), (m_featureID) );
        return false;
      }

      // Do not accumulate boundary types (boundary-administrative-*) for inner polygons.
      // Example: Minsk city border (admin_level=8) is inner for Minsk area border (admin_level=4).
      return (role != "inner");
    }

    uint32_t const m_boundaryType;
    uint32_t GetSkipBoundaryType(RelationElement const * rel) const
    {
      return ((rel == 0 || IsAcceptBoundaryTypes(*rel)) ? 0 : m_boundaryType);
    }

  public:
    type_processor() : m_boundaryType(ftype::GetBoundaryType2())
    {
    }

    ~type_processor()
    {
      for (typename RelationCacheT::iterator i = m_typeCache.begin(); i != m_typeCache.end(); ++i)
        delete i->second.m_e;
    }

    /// Start process new feature.
    void Reset(uint64_t fID, FeatureParams * val)
    {
      m_featureID = fID;
      m_val = val;
    }

    /// 1. "initial relation" process
    int operator() (uint64_t id)
    {
      typename RelationCacheT::const_iterator i = m_typeCache.find(id);
      if (i != m_typeCache.end())
      {
        m_val->AddTypes(i->second.m_p, GetSkipBoundaryType(i->second.m_e));
        return -1;  // continue process relations
      }
      return 0;     // read relation from file (see next operator)
    }

    /// 2. "relation from file" process
    /// param[in] rel Get non-const reference to Swap inner data
    bool operator() (uint64_t id, RelationElement & rel)
    {
      string const type = rel.GetType();
      if (type == "multipolygon")
      {
        // we will process multipolygons later
        return false;
      }
      bool const isBoundary = (type == "boundary");

      // make XMLElement struct from relation's tags for GetNameAndType function.
      XMLElement e;
      MakeXMLElement(rel, e);

      // process types of relation and add them to m_val
      RelationValue val;
      ftype::GetNameAndType(&e, val.m_p);
      if (val.m_p.IsValid())
      {
        m_val->AddTypes(val.m_p, GetSkipBoundaryType(isBoundary ? &rel : 0));

        if (isBoundary)
        {
          val.m_e = new RelationElement();
          val.m_e->Swap(rel);
        }
      }

      m_typeCache[id] = val;

      // continue process relations
      return false;
    }
  } m_typeProcessor;


  bool ParseType(XMLElement * p, uint64_t & id, FeatureParams & params)
  {
    CHECK ( strings::to_uint64(p->attrs["id"], id), (p->attrs["id"]) );

    // try to get type from element tags
    ftype::GetNameAndType(p, params);

    // try to get type from relations tags
    m_typeProcessor.Reset(id, &params);

    if (p->name == "node" && !params.IsValid())
    {
      // additional process of nodes ONLY if there is no native types
      m_holder.ForEachRelationByNodeCached(id, m_typeProcessor);
    }
    else if (p->name == "way")
    {
      // always make additional process of ways
      m_holder.ForEachRelationByWayCached(id, m_typeProcessor);
    }

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

  void EmitLine(FeatureBuilderT & ft, FeatureParams params, osm::Id id)
  {
    if ((m_coastType != 0 && params.IsTypeExist(m_coastType)) ||
        feature::RemoveNoDrawableTypes(params.m_Types, feature::GEOM_LINE))
    {
      ft.SetLinear(params.m_reverseGeometry);
      ft.SetOsmId(id);
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

      osm::Id const osmID = osm::Id::Way(id);

      ft.SetOsmId(osmID);
      EmitArea(ft, params, [id, this] (FeatureBuilderT & ft)
      {
        multipolygon_holes_processor processor(id, this);
        m_holder.ForEachRelationByWay(id, processor);
        ft.SetAreaAddHoles(processor.GetHoles());
      });

      EmitLine(ft, params, osmID);
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

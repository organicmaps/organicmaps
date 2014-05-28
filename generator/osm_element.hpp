#pragma once

#include "osm2type.hpp"
#include "xml_element.hpp"
#include "feature_builder.hpp"
#include "ways_merger.hpp"

#include "../search/ftypes_matcher.hpp"

#include "../indexer/feature_visibility.hpp"
#include "../indexer/classificator.hpp"

#include "../base/string_utils.hpp"
#include "../base/logging.hpp"
#include "../base/stl_add.hpp"

#include "../std/unordered_map.hpp"
#include "../std/list.hpp"


/// @param  TEmitter  Feature accumulating policy
/// @param  THolder   Nodes, ways, relations holder
template <class TEmitter, class THolder>
class SecondPassParserBase : public BaseOSMParser
{
protected:
  TEmitter & m_emitter;
  THolder & m_holder;

  SecondPassParserBase(TEmitter & emitter, THolder & holder)
    : m_emitter(emitter), m_holder(holder)
  {
    static char const * tags[] = { "osm", "node", "way", "relation" };
    SetTags(tags);
  }

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
    holes_accumulator(SecondPassParserBase * pMain) : m_merger(pMain->m_holder) {}

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
    multipolygon_holes_processor(uint64_t id, SecondPassParserBase * pMain)
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
    void make_xml_element(RelationElement const & rel, XMLElement & out)
    {
      for (map<string, string>::const_iterator i = rel.tags.begin(); i != rel.tags.end(); ++i)
      {
        if (i->first == "type") continue;

        out.childs.push_back(XMLElement());
        XMLElement & e = out.childs.back();
        e.name = "tag";
        e.attrs["k"] = i->first;
        e.attrs["v"] = i->second;
      }
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
      make_xml_element(rel, e);

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

  typedef FeatureBuilder1 feature_builder_t;

  void FinishAreaFeature(uint64_t id, feature_builder_t & ft)
  {
    ASSERT ( ft.IsGeometryClosed(), () );

    multipolygon_holes_processor processor(id, this);
    m_holder.ForEachRelationByWay(id, processor);
    ft.SetAreaAddHoles(processor.GetHoles());
  }

  class UselessSingleTypes
  {
    vector<uint32_t> m_types;

    bool IsUseless(uint32_t t) const
    {
      return (find(m_types.begin(), m_types.end(), t) != m_types.end());
    }

  public:
    UselessSingleTypes()
    {
      Classificator const & c = classif();

      char const * arr[][1] = { { "oneway" }, { "lit" } };
      for (size_t i = 0; i < ARRAY_SIZE(arr); ++i)
        m_types.push_back(c.GetTypeByPath(vector<string>(arr[i], arr[i] + 1)));
    }

    bool IsValid(vector<uint32_t> const & types) const
    {
      size_t const count = types.size();
      size_t uselessCount = 0;
      for (size_t i = 0; i < count; ++i)
        if (IsUseless(types[i]))
          ++uselessCount;
      return (count > uselessCount);
    }
  };

  bool ParseType(XMLElement * p, uint64_t & id, FeatureParams & fValue)
  {
    CHECK ( strings::to_uint64(p->attrs["id"], id), (p->attrs["id"]) );

    // try to get type from element tags
    ftype::GetNameAndType(p, fValue);

    // try to get type from relations tags
    m_typeProcessor.Reset(id, &fValue);

    if (p->name == "node" && !fValue.IsValid())
    {
      // additional process of nodes ONLY if there is no native types
      m_holder.ForEachRelationByNodeCached(id, m_typeProcessor);
    }
    else if (p->name == "way")
    {
      // always make additional process of ways
      m_holder.ForEachRelationByWayCached(id, m_typeProcessor);
    }

    fValue.FinishAddingTypes();

    // unrecognized feature by classificator
    static UselessSingleTypes checker;
    return fValue.IsValid() && checker.IsValid(fValue.m_Types);
  }

  class multipolygons_emitter
  {
    SecondPassParserBase * m_pMain;
    FeatureParams const & m_params;
    holes_list_t & m_holes;
    uint64_t m_relID;

  public:
    multipolygons_emitter(SecondPassParserBase * pMain,
                          FeatureParams const & params,
                          holes_list_t & holes,
                          uint64_t relID)
      : m_pMain(pMain), m_params(params), m_holes(holes), m_relID(relID)
    {
    }

    void operator() (pts_vec_t const & pts, vector<uint64_t> const & ids)
    {
      feature_builder_t f;
      f.SetParams(m_params);

      for (size_t i = 0; i < ids.size(); ++i)
        f.AddOsmId("way", ids[i]);

      for (size_t i = 0; i < pts.size(); ++i)
        f.AddPoint(pts[i]);

      if (f.IsGeometryClosed())
      {
        f.SetAreaAddHoles(m_holes);
        if (f.PreSerialize())
        {
          f.AddOsmId("relation", m_relID);
          m_pMain->m_emitter(f);
        }
      }
    }
  };
};

template <class TEmitter, class THolder>
class SecondPassParserUsual : public SecondPassParserBase<TEmitter, THolder>
{
  typedef SecondPassParserBase<TEmitter, THolder> base_type;

  typedef typename base_type::feature_builder_t feature_t;

  uint32_t m_coastType;

  scoped_ptr<FileWriter> m_addrWriter;
  bool NeedWriteAddress(FeatureParams const & params) const
  {
    static ftypes::IsBuildingChecker checker;
    return m_addrWriter && checker(params.m_Types);
  }

protected:
  virtual void EmitElement(XMLElement * p)
  {
    using namespace feature;

    uint64_t id;
    FeatureParams fValue;
    if (!base_type::ParseType(p, id, fValue))
      return;

    feature_t ft;

    if (p->name == "node")
    {
      if (!feature::RemoveNoDrawableTypes(fValue.m_Types, FEATURE_TYPE_POINT))
        return;

      m2::PointD pt;
      if (p->childs.empty() || !base_type::GetPoint(id, pt))
        return;

      ft.SetCenter(pt);
    }
    else if (p->name == "way")
    {
      // It's useless here to skip any types by drawing criteria.
      // Area features can combine all drawing types (point, linear, unique area).

      // geometry of feature
      for (size_t i = 0; i < p->childs.size(); ++i)
      {
        if (p->childs[i].name == "nd")
        {
          uint64_t nodeID;
          CHECK ( strings::to_uint64(p->childs[i].attrs["ref"], nodeID), (p->childs[i].attrs["ref"]) );

          m2::PointD pt;
          if (!base_type::GetPoint(nodeID, pt))
            return;

          ft.AddPoint(pt);
        }
      }

      size_t const count = ft.GetPointsCount();
      if (count < 2)
        return;

      bool const isClosed = (count > 2 && ft.IsGeometryClosed());
      // Try to set area feature (point and linear types are also suitable for this)
      if (isClosed && feature::IsDrawableLike(fValue.m_Types, FEATURE_TYPE_AREA))
        base_type::FinishAreaFeature(id, ft);
      else
      {
        if (isClosed)
        {
          // Make point feature (in center) if geometry is closed and has point drawing rules.
          FeatureParams params(fValue);
          if (feature::RemoveNoDrawableTypes(params.m_Types, FEATURE_TYPE_POINT))
          {
            feature_t f;
            f.SetParams(params);
            f.SetCenter(ft.GetGeometryCenter());
            if (f.PreSerialize())
            {
              string addr;
              if (NeedWriteAddress(params) && f.FormatFullAddress(addr))
                m_addrWriter->Write(addr.c_str(), addr.size());

              base_type::m_emitter(f);
            }
          }
        }

        // Try to set linear feature:
        // - it's a coastline, OR
        // - has linear types (remove others)
        if ((m_coastType != 0 && fValue.IsTypeExist(m_coastType)) ||
            feature::RemoveNoDrawableTypes(fValue.m_Types, FEATURE_TYPE_LINE))
        {
          ft.SetLinear();
        }
        else
          return;
      }
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

      // 2. Relation should have visible area types.
      if (!feature::IsDrawableLike(fValue.m_Types, FEATURE_TYPE_AREA))
        return;

      typename base_type::holes_accumulator holes(this);
      AreaWayMerger<THolder> outer(base_type::m_holder);

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

      typename base_type::multipolygons_emitter emitter(this, fValue, holes.GetHoles(), id);
      outer.ForEachArea(emitter, true);
      return;
    }

    ft.SetParams(fValue);
    if (ft.PreSerialize())
    {
      string addr;
      if (NeedWriteAddress(fValue) && ft.FormatFullAddress(addr))
        m_addrWriter->Write(addr.c_str(), addr.size());

      // add osm id for debugging
      ft.AddOsmId(p->name, id);
      base_type::m_emitter(ft);
    }
  }

public:
  SecondPassParserUsual(TEmitter & emitter, THolder & holder,
                        uint32_t coastType, string const & addrFilePath)
    : base_type(emitter, holder), m_coastType(coastType)
  {
    if (!addrFilePath.empty())
      m_addrWriter.reset(new FileWriter(addrFilePath));
  }
};

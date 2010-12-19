#pragma once

#include "../../indexer/feature.hpp"
#include "../../indexer/osm2type.hpp"
#include "../../indexer/xml_element.hpp"
#include "../../indexer/osm_decl.hpp"
#include "../../indexer/feature_visibility.hpp"

#include "../../base/string_utils.hpp"
#include "../../base/logging.hpp"

#include "../../std/unordered_map.hpp"
#include "../../std/list.hpp"
#include "../../std/set.hpp"
#include "../../std/vector.hpp"

#include "../../base/start_mem_debug.hpp"

namespace feature
{
  typedef list<vector<m2::PointD> > holes_cont_t;
  void TesselateInterior(FeatureBuilderGeom & featureBuilder, feature::holes_cont_t const & holes);
}

/// @param  TEmitter  Feature accumulating policy
/// @param  THolder   Nodes, ways, relations holder
template <class TEmitter, class THolder>
class SecondPassParserBase : public BaseOSMParser
{
protected:
  TEmitter & m_emitter;
  THolder & m_holder;

  /// max possible number of types per feature
  static const size_t max_number_of_types = 16;

  SecondPassParserBase(TEmitter & emitter, THolder & holder)
    : m_emitter(emitter), m_holder(holder)
  {
    static char const * tags[] = { "osm", "node", "way" };
    SetTags(tags);
  }

  /// Finding of "holes" in area objects (multipolygon).
  class multipolygon_processor
  {
    uint64_t m_id;      ///< id of way to find it's holes
    THolder & m_holder;

  public:
    /// @param[out] list of holes
    feature::holes_cont_t m_holes;

    multipolygon_processor(uint64_t id, THolder & holder) : m_id(id), m_holder(holder) {}

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
      {
        WayElement e;
        if (m_holder.GetWay(id, e))
        {
          m_holes.push_back(vector<m2::PointD>());
          e.ForEachPoint(*this);
        }
      }
    }
    /// 3. "points in way" process function
    void operator () (uint64_t id)
    {
      double lat, lng;
      if (m_holder.GetNode(id, lat, lng))
      {
        // lng is "x", lat is "y" - see Feature::AddPoint
        m_holes.back().push_back(m2::PointD(lng, lat));
      }
    }
  };

  /// Feature description struct.
  struct value_t
  {
    typedef vector<uint32_t> types_t;
    types_t types;    ///< 1-n types, not empty
    string name;      ///< 1-1 name, @todo 1-n names
    int32_t layer;    ///< layer

    value_t()
    {
      types.reserve(max_number_of_types);
    }
    bool IsValid() const { return !types.empty(); }
    void Add(value_t const & v)
    {
      types.insert(types.end(), v.types.begin(), v.types.end());
    }
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

    uint64_t m_featureID;
    /// @param[out] feature value as result
    value_t * m_val;

    /// cache: relation id -> feature value (for fast feature parsing)
    unordered_map<uint64_t, value_t> m_typeCache;

  public:
    void Reset(uint64_t fID, value_t * val)
    {
      m_featureID = fID;
      m_val = val;
    }

    /// 1. "initial relation" process
    int operator() (uint64_t id)
    {
      typename unordered_map<uint64_t, value_t>::const_iterator i = m_typeCache.find(id);
      if (i != m_typeCache.end())
      {
        m_val->Add(i->second);
        return -1;  // continue process relations
      }
      return 0;     // read relation from file (see next operator)
    }

    /// 2. "relation from file" process
    bool operator() (uint64_t id, RelationElement const & rel)
    {
      // "multipolygon relations" process only for "outer" way-features
      if (rel.GetType() == "multipolygon")
      {
        string role;
        if (rel.FindWay(m_featureID, role)) // feature is way ...
          if (role != "outer")
            return false;
      }

      // make XMLElement struct from relation's tags for GetNameAndType function.
      XMLElement e;
      make_xml_element(rel, e);

      value_t val;
      if (ftype::GetNameAndType(&e, val.types, val.name, val.layer))
      {
        m_typeCache[id] = val;
        m_val->Add(val);
      }
      else
        m_typeCache[id] = value_t();

      // continue process relations
      return false;
    }
  } m_typeProcessor;

  typedef FeatureBuilderGeom feature_builder_t;

  bool GetPoint(uint64_t id, m2::PointD & pt)
  {
    return m_holder.GetNode(id, pt.y, pt.x);
  }

  void FinishAreaFeature(uint64_t id, feature_builder_t & ft)
  {
    if (ft.IsGeometryClosed())
    {
      multipolygon_processor processor(id, m_holder);
      m_holder.ForEachRelationByWay(id, processor);
      feature::TesselateInterior(ft, processor.m_holes);
    }
  }

  bool ParseType(XMLElement * p, uint64_t & id, value_t & fValue)
  {
    VERIFY ( utils::to_uint64(p->attrs["id"], id),
      ("Unknown element with invalid id : ", p->attrs["id"]) );

    // try to get type from element tags
    ftype::GetNameAndType(p, fValue.types, fValue.name, fValue.layer);

    // try to get type from relations tags
    m_typeProcessor.Reset(id, &fValue);
    m_holder.ForEachRelationByWayCached(id, m_typeProcessor);

    // remove duplicating types
    sort(fValue.types.begin(), fValue.types.end());
    fValue.types.erase(unique(fValue.types.begin(), fValue.types.end()), fValue.types.end());

    // unrecognized feature by classificator
    return fValue.IsValid();
  }
};

template <class TEmitter, class THolder>
class SecondPassParserJoin : public SecondPassParserBase<TEmitter, THolder>
{
  typedef SecondPassParserBase<TEmitter, THolder> base_type;

  set<uint64_t> m_usedDirect;

  bool TryEmitUnited(uint64_t featureID, typename base_type::feature_builder_t & ft)
  {
    // check, if feature already processed early
    if (m_usedDirect.count(featureID) > 0)
      return true;

    set<uint64_t> path;
    path.insert(featureID);

    WayElement e;

    // process geometry of initial way itself
    base_type::m_holder.GetWay(featureID, e);
    if (e.nodes.empty())
      return false;

    for (size_t i = 0; i < e.nodes.size(); ++i)
    {
      m2::PointD pt;
      if (base_type::GetPoint(e.nodes[i], pt))
        ft.AddPoint(pt);
      else
        return false;
    }

    // process connected ways in cycle while ...
    uint64_t fID = featureID;
    while (!ft.IsGeometryClosed())
    {
      uint64_t const nodeID = e.nodes.back();
      if (!base_type::m_holder.GetNextWay(fID, nodeID, e))
        break;

      if (!path.insert(fID).second)
      {
        LOG_SHORT(LWARNING, ("JOIN_DBG! Cycle found during way joining, duplicate id = ", fID));
        break;
      }

      // skip first point, because it's equal with previous
      size_t i;
      int inc;
      if (e.nodes.front() == nodeID)
      {
        i = 1;
        inc = 1;
      }
      else
      {
        ASSERT ( e.nodes.back() == nodeID, () );

        i = e.nodes.size() - 2;
        inc = -1;
      }

      size_t count = 1;
      while (count++ < e.nodes.size())
      {
        m2::PointD pt;
        if (base_type::GetPoint(e.nodes[i], pt))
          ft.AddPoint(pt);
        else
          return false;

        i += inc;
      }
    }

    if (ft.IsGeometryClosed())
    {
      m_usedDirect.insert(path.begin(), path.end());

      base_type::FinishAreaFeature(featureID, ft);

      base_type::m_emitter(ft);
      return true;
    }
    else
    {
      LOG_SHORT(LWARNING, ("JOIN_DBG! Ways not connected for root way = ", featureID));
      return false;
    }
  }

protected:
  virtual void EmitElement(XMLElement * p)
  {
    uint64_t id;
    typename base_type::value_t fValue;
    if (!ParseType(p, id, fValue))
      return;

    // check, if we can make united feature
    for (typename base_type::value_t::types_t::iterator i = fValue.types.begin(); i != fValue.types.end(); ++i)
      if (feature::NeedUnite(*i))
      {
        typename base_type::feature_builder_t ft;
        ft.AddName(fValue.name);
        ft.AddTypes(fValue.types.begin(), fValue.types.end());
        ft.AddLayer(fValue.layer);

        TryEmitUnited(id, ft);
        break;
      }
  }

public:
  SecondPassParserJoin(TEmitter & emitter, THolder & holder)
    : base_type(emitter, holder)
  {
  }
};

template <class TEmitter, class THolder>
class SecondPassParserUsual : public SecondPassParserBase<TEmitter, THolder>
{
  typedef SecondPassParserBase<TEmitter, THolder> base_type;

protected:
  virtual void EmitElement(XMLElement * p)
  {
    uint64_t id;
    typename base_type::value_t fValue;
    if (!ParseType(p, id, fValue))
      return;

    typename base_type::feature_builder_t ft;
    ft.AddName(fValue.name);
    ft.AddTypes(fValue.types.begin(), fValue.types.end());
    ft.AddLayer(fValue.layer);

    if (p->name == "node")
    {
      if (!feature::IsDrawableLike(fValue.types, feature::fpoint))
        return;

      m2::PointD pt;
      if (p->childs.empty() || !base_type::GetPoint(id, pt))
        return;

      ft.AddPoint(pt);
    }
    else if (p->name == "way")
    {
      bool const isArea = feature::IsDrawableLike(fValue.types, feature::farea);

      if (!feature::IsDrawableLike(fValue.types, feature::fline) && !isArea)
        return;

      // geometry of feature
      for (size_t i = 0; i < p->childs.size(); ++i)
      {
        if (p->childs[i].name == "nd")
        {
          uint64_t id;
          VERIFY ( utils::to_uint64(p->childs[i].attrs["ref"], id),
                   ("Bad node ref in way : ", p->childs[i].attrs["ref"]) );

          m2::PointD pt;
          if (!base_type::GetPoint(id, pt))
            return;

          ft.AddPoint(pt);
        }
      }

      if (ft.GetPointsCount() <= 1)
        return;

      // Get the tesselation for an area object (only if it has area drawing rules,
      // otherwise it will stay a linear object).
      if (isArea)
        base_type::FinishAreaFeature(id, ft);
    }
    else
    {
      ASSERT ( false, ("Unknown osm element : ", p->name) );
      return;
    }

    base_type::m_emitter(ft);
  }

public:
  SecondPassParserUsual(TEmitter & emitter, THolder & holder)
    : base_type(emitter, holder)
  {
  }
};

#include "../../base/stop_mem_debug.hpp"

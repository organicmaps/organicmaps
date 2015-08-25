#pragma once

#include "generator/osm2type.hpp"
#include "generator/osm_element.hpp"
#include "generator/feature_builder.hpp"
#include "generator/ways_merger.hpp"

#include "indexer/ftypes_matcher.hpp"
#include "indexer/feature_visibility.hpp"
#include "indexer/classificator.hpp"

#include "geometry/tree4d.hpp"

#include "base/string_utils.hpp"
#include "base/logging.hpp"
#include "base/stl_add.hpp"
#include "base/cache.hpp"

#include "std/unordered_set.hpp"
#include "std/list.hpp"


/// @param  TEmitter  Feature accumulating policy
/// @param  TCache   Nodes, ways, relations holder
template <class TEmitter, class TCache>
class OsmToFeatureTranslator
{
  TEmitter & m_emitter;
  TCache & m_holder;


  bool GetPoint(uint64_t id, m2::PointD & pt) const
  {
    return m_holder.GetNode(id, pt.y, pt.x);
  }

  typedef vector<m2::PointD> pts_vec_t;
  typedef list<pts_vec_t> holes_list_t;

  class holes_accumulator
  {
    AreaWayMerger<TCache> m_merger;
    holes_list_t m_holes;

  public:
    holes_accumulator(OsmToFeatureTranslator * pMain) : m_merger(pMain->m_holder) {}

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
    multipolygon_holes_processor(uint64_t id, OsmToFeatureTranslator * pMain)
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
  class RelationTagsBase
  {
  public:
    RelationTagsBase() : m_cache(14) {}

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

  protected:
    static bool IsSkipRelation(string const & type)
    {
      /// @todo Skip special relation types.
      return (type == "multipolygon" || type == "bridge" || type == "restriction");
    }

    bool IsKeyTagExists(string const & key) const
    {
      for (auto const & p : m_current->m_tags)
        if (p.key == key)
          return true;
      return false;
    }

    virtual void Process(RelationElement const & e) = 0;

  protected:
    uint64_t m_featureID;
    OsmElement * m_current;

  private:
    my::Cache<uint64_t, RelationElement> m_cache;
  };

  class RelationTagsNode : public RelationTagsBase
  {
    typedef RelationTagsBase TBase;

  protected:
    void Process(RelationElement const & e) override
    {
      if (TBase::IsSkipRelation(e.GetType()))
        return;

      for (auto const & p : e.tags)
      {
        // Store only this tags to use it in railway stations processing for the particular city.
        if (p.first == "network" || p.first == "operator" || p.first == "route")
          if (!TBase::IsKeyTagExists(p.first))
            TBase::m_current->AddTag(p.first, p.second);
      }
    }
  } m_nodeRelations;

  class RelationTagsWay : public RelationTagsBase
  {
    typedef RelationTagsBase TBase;

    bool IsAcceptBoundary(RelationElement const & e) const
    {
      string role;
      CHECK(e.FindWay(TBase::m_featureID, role), (TBase::m_featureID));

      // Do not accumulate boundary types (boundary=administrative) for inner polygons.
      // Example: Minsk city border (admin_level=8) is inner for Minsk area border (admin_level=4).
      return (role != "inner");
    }

    typedef unordered_set<string> NameKeysT;
    void GetNameKeys(NameKeysT & keys) const
    {
      for (auto const & p : TBase::m_current->m_tags)
        if (strings::StartsWith(p.key, "name"))
          keys.insert(p.key);
    }

  protected:
    void Process(RelationElement const & e) override
    {
      /// @todo Review route relations in future.
      /// Actually, now they give a lot of dummy tags.
      string const type = e.GetType();
      if (TBase::IsSkipRelation(type) || type == "route")
        return;

      bool const isWay = (TBase::m_current->type == OsmElement::EntityType::Way);
      bool const isBoundary = isWay && (type == "boundary") && IsAcceptBoundary(e);

      NameKeysT nameKeys;
      GetNameKeys(nameKeys);

      for (auto const & p : e.tags)
      {
        /// @todo Skip common key tags.
        if (p.first == "type" || p.first == "route")
          continue;

        // Skip already existing "name" tags.
        if (nameKeys.count(p.first) != 0)
          continue;

        if (!isBoundary && p.first == "boundary")
          continue;

        if (isWay && p.first == "place")
          continue;

        TBase::m_current->AddTag(p.first, p.second);
      }
    }

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
    params.FinishAddingTypes();
    return ftype::IsValidTypes(params);
  }

  typedef FeatureBuilder1 FeatureBuilderT;

  class multipolygons_emitter
  {
    OsmToFeatureTranslator * m_pMain;
    FeatureParams const & m_params;
    holes_list_t & m_holes;
    uint64_t m_relID;

  public:
    multipolygons_emitter(OsmToFeatureTranslator * pMain,
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
    return m_addrWriter && ftypes::IsBuildingChecker::Instance()(params.m_Types);
  }

  class Place
  {
    FeatureBuilderT m_ft;
    m2::PointD m_pt;
    uint32_t m_type;

    static constexpr double EQUAL_PLACE_SEARCH_RADIUS_M = 20000.0;

    bool IsPoint() const { return (m_ft.GetGeomType() == feature::GEOM_POINT); }
    static bool IsEqualTypes(uint32_t t1, uint32_t t2)
    {
      // Use 2-arity places comparison for filtering.
      // ("place-city-capital-2" is equal to "place-city")
      ftype::TruncValue(t1, 2);
      ftype::TruncValue(t2, 2);
      return (t1 == t2);
    }

  public:
    Place(FeatureBuilderT const & ft, uint32_t type)
      : m_ft(ft), m_pt(ft.GetKeyPoint()), m_type(type)
    {
    }

    FeatureBuilderT const & GetFeature() const { return m_ft; }

    m2::RectD GetLimitRect() const
    {
      return MercatorBounds::RectByCenterXYAndSizeInMeters(m_pt, EQUAL_PLACE_SEARCH_RADIUS_M);
    }

    bool IsEqual(Place const & r) const
    {
      return (IsEqualTypes(m_type, r.m_type) &&
              m_ft.GetName() == r.m_ft.GetName() &&
              (IsPoint() || r.IsPoint()) &&
              MercatorBounds::DistanceOnEarth(m_pt, r.m_pt) < EQUAL_PLACE_SEARCH_RADIUS_M);
    }

    /// Check whether we need to replace place @r with place @this.
    bool IsBetterThan(Place const & r) const
    {
      // Area places has priority before point places.
      if (!r.IsPoint())
        return false;
      if (!IsPoint())
        return true;

      // Check types length.
      // ("place-city-capital-2" is better than "place-city").
      uint8_t const l1 = ftype::GetLevel(m_type);
      uint8_t const l2 = ftype::GetLevel(r.m_type);
      if (l1 != l2)
        return (l2 < l1);

      // Check ranks.
      return (r.m_ft.GetRank() < m_ft.GetRank());
    }
  };

  m4::Tree<Place> m_places;

  void EmitFeatureBase(FeatureBuilderT & ft, FeatureParams const & params)
  {
    ft.SetParams(params);
    if (ft.PreSerialize())
    {
      string addr;
      if (NeedWriteAddress(params) && ft.FormatFullAddress(addr))
        m_addrWriter->Write(addr.c_str(), addr.size());

      static uint32_t const placeType = classif().GetTypeByPath({"place"});
      uint32_t const type = params.FindType(placeType, 1);

      if (type != ftype::GetEmptyValue() && !ft.GetName().empty())
      {
        m_places.ReplaceEqualInRect(Place(ft, type),
                                    bind(&Place::IsEqual, _1, _2),
                                    bind(&Place::IsBetterThan, _1, _2));
      }
      else
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

public:
  /// The main entry point for parsing process.
  void EmitElement(OsmElement * p)
  {
    if (p->type == OsmElement::EntityType::Node)
    {
      if (p->m_tags.empty())
        return;

      FeatureParams params;
      if (!ParseType(p, params))
        return;

      m2::PointD const pt = MercatorBounds::FromLatLon(p->lat, p->lon);
      EmitPoint(pt, params, osm::Id::Node(p->id));
    }
    else if (p->type == OsmElement::EntityType::Way)
    {
      FeatureBuilderT ft;

      // Parse geometry.
      for (uint64_t ref : p->Nodes())
      {
        m2::PointD pt;
        if (!GetPoint(ref, pt))
          return;

        ft.AddPoint(pt);
      }

      if (ft.GetPointsCount() < 2)
        return;

      FeatureParams params;
      if (!ParseType(p, params))
        return;

      ft.SetOsmId(osm::Id::Way(p->id));
      bool isCoastLine = (m_coastType != 0 && params.IsTypeExist(m_coastType));

      EmitArea(ft, params, [&] (FeatureBuilderT & ft)
      {
        isCoastLine = false;  // emit coastline feature only once
        multipolygon_holes_processor processor(p->id, this);
        m_holder.ForEachRelationByWay(p->id, processor);
        ft.SetAreaAddHoles(processor.GetHoles());
      });

      EmitLine(ft, params, isCoastLine);
    }
    else if (p->type == OsmElement::EntityType::Relation)
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
          return;
      }

      FeatureParams params;
      if (!ParseType(p, params))
        return;

      holes_accumulator holes(this);
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

      multipolygons_emitter emitter(this, params, holes.GetHoles(), p->id);
      outer.ForEachArea(emitter, true);
    }
  }

public:
  OsmToFeatureTranslator(TEmitter & emitter, TCache & holder,
                   uint32_t coastType, string const & addrFilePath)
    : m_emitter(emitter), m_holder(holder), m_coastType(coastType)
  {
    if (!addrFilePath.empty())
      m_addrWriter.reset(new FileWriter(addrFilePath));
  }

  void Finish()
  {
    m_places.ForEach([this] (Place const & p)
    {
      m_emitter(p.GetFeature());
    });
  }
};

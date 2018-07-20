#include "generator/osm_translator.hpp"

namespace generator {

// RelationTagsBase
RelationTagsBase::RelationTagsBase(routing::TagsProcessor & tagsProcessor)
  : m_routingTagsProcessor(tagsProcessor), m_cache(14)
{
}

void RelationTagsBase::Reset(uint64_t fID, OsmElement * p)
{
  m_featureID = fID;
  m_current = p;
}

bool RelationTagsBase::IsSkipRelation(std::string const & type)
{
  /// @todo Skip special relation types.
  return (type == "multipolygon" || type == "bridge");
}

bool RelationTagsBase::IsKeyTagExists(std::string const & key) const
{
  for (auto const & p : m_current->m_tags)
    if (p.key == key)
      return true;
  return false;
}

void RelationTagsBase::AddCustomTag(pair<std::string, std::string> const & p)
{
  m_current->AddTag(p.first, p.second);
}

// RelationTagsNode
RelationTagsNode::RelationTagsNode(routing::TagsProcessor & tagsProcessor) :
  RelationTagsBase(tagsProcessor)
{
}

void RelationTagsNode::Process(RelationElement const & e)
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
                                       TBase::IsKeyTagExists("addr:housenumber") &&
                                       !TBase::IsKeyTagExists("addr:street");
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


// RelationTagsWay
RelationTagsWay::RelationTagsWay(routing::TagsProcessor & routingTagsProcessor)
  : RelationTagsBase(routingTagsProcessor)
{
}


bool RelationTagsWay::IsAcceptBoundary(RelationElement const & e) const
{
  std::string role;
  CHECK(e.FindWay(TBase::m_featureID, role), (TBase::m_featureID));

  // Do not accumulate boundary types (boundary=administrative) for inner polygons.
  // Example: Minsk city border (admin_level=8) is inner for Minsk area border (admin_level=4).
  return (role != "inner");
}


void RelationTagsWay::Process(RelationElement const & e)
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

// HolesAccumulator
HolesAccumulator::HolesAccumulator(cache::IntermediateDataReader & holder) :
  m_merger(holder)
{
}

FeatureBuilder1::Geometry & HolesAccumulator::GetHoles()
{
  ASSERT(m_holes.empty(), ("Can call only once"));
  m_merger.ForEachArea(false, [this](FeatureBuilder1::PointSeq const & v, std::vector<uint64_t> const &)
  {
    m_holes.push_back(std::move(v));
  });
  return m_holes;
}


// HolesProcessor
HolesProcessor::HolesProcessor(uint64_t id, cache::IntermediateDataReader & holder)
  : m_id(id)
  , m_holes(holder)
{
}

bool HolesProcessor::operator() (uint64_t /*id*/, RelationElement const & e)
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

void HolesProcessor::operator() (uint64_t id, std::string const & role)
{
  if (id != m_id && role == "inner")
    m_holes(id);
}


// OsmToFeatureTranslator
OsmToFeatureTranslator::OsmToFeatureTranslator(std::shared_ptr<EmitterBase> emitter,
                                               cache::IntermediateDataReader & holder,
                                               const feature::GenerateInfo & info) :
  m_emitter(emitter),
  m_holder(holder),
  m_coastType(info.m_makeCoasts ? classif().GetCoastType() : 0),
  m_nodeRelations(m_routingTagsProcessor),
  m_wayRelations(m_routingTagsProcessor),
  m_metalinesBuilder(info.GetIntermediateFileName(METALINES_FILENAME))
{
  auto const addrFilePath = info.GetAddressesFileName();
  if (!addrFilePath.empty())
    m_addrWriter.reset(new FileWriter(addrFilePath));

  auto const restrictionsFilePath = info.GetIntermediateFileName(RESTRICTIONS_FILENAME);
  if (!restrictionsFilePath.empty())
    m_routingTagsProcessor.m_restrictionWriter.Open(restrictionsFilePath);

  auto const roadAccessFilePath = info.GetIntermediateFileName(ROAD_ACCESS_FILENAME);
  if (!roadAccessFilePath.empty())
    m_routingTagsProcessor.m_roadAccessWriter.Open(roadAccessFilePath);
}


void OsmToFeatureTranslator::EmitElement(OsmElement * p)
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
      HolesProcessor processor(p->id, m_holder);
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

    HolesAccumulator holes(m_holder);
    AreaWayMerger outer(m_holder);

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
    outer.ForEachArea(true, [&] (FeatureBuilder1::PointSeq const & pts, std::vector<uint64_t> const & ids)
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
}

bool OsmToFeatureTranslator::ParseType(OsmElement * p, FeatureParams & params)
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

void OsmToFeatureTranslator::EmitPoint(m2::PointD const & pt,
                                       FeatureParams params, osm::Id id) const
{
  if (feature::RemoveNoDrawableTypes(params.m_Types, feature::GEOM_POINT))
  {
    FeatureBuilder1 ft;
    ft.SetCenter(pt);
    ft.SetOsmId(id);
    EmitFeatureBase(ft, params);
  }
}

void OsmToFeatureTranslator::EmitLine(FeatureBuilder1 & ft, FeatureParams params,
                                      bool isCoastLine) const
{
  if (isCoastLine || feature::RemoveNoDrawableTypes(params.m_Types, feature::GEOM_LINE))
  {
    ft.SetLinear(params.m_reverseGeometry);
    EmitFeatureBase(ft, params);
  }
}

void OsmToFeatureTranslator::EmitFeatureBase(FeatureBuilder1 & ft, FeatureParams const & params) const
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

    (*m_emitter)(ft);
  }
}


// OsmToFeatureTranslatorRegion
OsmToFeatureTranslatorRegion::OsmToFeatureTranslatorRegion(std::shared_ptr<EmitterBase> emitter,
                                                           cache::IntermediateDataReader & holder)
  : m_emitter(emitter)
  , m_holder(holder)
{
}

void OsmToFeatureTranslatorRegion::EmitElement(OsmElement * p)
{
  CHECK(p, ("Tried to emit a null OsmElement"));

  switch(p->type)
  {
  case OsmElement::EntityType::Relation:
  {
    FeatureParams params;
    if (!(IsSuitableElement(p) && ParseParams(p, params)))
      return;

    BuildFeatureAndEmit(p, params);
    break;
  }
  default:
    break;
  }
}

bool OsmToFeatureTranslatorRegion::IsSuitableElement(OsmElement const * p) const
{
  static std::set<string> const adminLevels = {"2", "4", "5", "6", "7", "8"};
  static std::set<string> const places = {"city", "town", "village", "suburb", "neighbourhood",
                                          "hamlet", "locality", "isolated_dwelling"};

  bool haveBoundary = false;
  bool haveAdminLevel = false;
  bool haveName = false;
  for (auto const & t : p->Tags())
  {    if (t.key == "place" && places.find(t.value) != places.end())
      return true;

    if (t.key == "boundary" && t.value == "administrative")
      haveBoundary = true;
    else if (t.key == "admin_level" && adminLevels.find(t.value) != adminLevels.end())
      haveAdminLevel = true;
    else if (t.key == "name" && !t.value.empty())
      haveName = true;

    if (haveBoundary && haveAdminLevel && haveName)
      return true;
  }

  return false;
}

void OsmToFeatureTranslatorRegion::AddInfoAboutRegion(OsmElement const * p,
                                                      FeatureBuilder1 & ft) const
{
  cout << 1;
}

bool OsmToFeatureTranslatorRegion::ParseParams(OsmElement * p, FeatureParams & params) const
{
  ftype::GetNameAndType(p, params);
  return params.IsValid();
}

void OsmToFeatureTranslatorRegion::BuildFeatureAndEmit(OsmElement const * p, FeatureParams & params)
{
  HolesAccumulator holes(m_holder);
  AreaWayMerger outer(m_holder);
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
  outer.ForEachArea(true, [&] (FeatureBuilder1::PointSeq const & pts,
                    std::vector<uint64_t> const & ids)
  {
    FeatureBuilder1 ft;
    for (uint64_t id : ids)
      ft.AddOsmId(osm::Id::Way(id));

    for (auto const & pt : pts)
      ft.AddPoint(pt);

    ft.AddOsmId(osm::Id::Relation(p->id));
    if (!ft.IsGeometryClosed())
      return;

    ft.SetAreaAddHoles(holesGeometry);
    ft.SetParams(params);
    if (!ft.PreSerialize())
      return;

    AddInfoAboutRegion(p, ft);
    (*m_emitter)(ft);
  });
}

}  // namespace generator

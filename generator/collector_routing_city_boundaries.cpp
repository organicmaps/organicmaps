#include "generator/collector_routing_city_boundaries.hpp"

#include "generator/intermediate_data.hpp"
#include "generator/osm_element.hpp"
#include "generator/osm_element_helpers.hpp"

#include "indexer/feature_algo.hpp"
#include "indexer/ftypes_matcher.hpp"

#include "geometry/mercator.hpp"

#include "coding/read_write_utils.hpp"

#include "base/assert.hpp"

#include <algorithm>
#include <iterator>

namespace generator
{
using namespace feature;
using ftypes::LocalityType;

// PlaceBoundariesHolder::Locality ----------------------------------------------------------------

PlaceBoundariesHolder::Locality::Locality(std::string const & placeType, OsmElement const & elem)
  : m_adminLevel(osm_element::GetAdminLevel(elem))
  , m_name(elem.GetTag("name"))
  , m_population(osm_element::GetPopulation(elem))
  , m_place(ftypes::LocalityFromString(placeType))
{}

template <class Sink>
void PlaceBoundariesHolder::Locality::Serialize(Sink & sink) const
{
  CHECK(TestValid(), ());

  WriteToSink(sink, static_cast<int8_t>(m_place));
  WriteToSink(sink, static_cast<int8_t>(m_placeFromNode));
  WriteVarUint(sink, m_population);
  WriteVarUint(sink, m_populationFromNode);
  WriteToSink(sink, m_adminLevel);

  rw::WritePOD(sink, m_center);

  // Same as FeatureBuilder::SerializeAccuratelyForIntermediate.
  WriteVarUint(sink, static_cast<uint32_t>(m_boundary.size()));
  for (auto const & e : m_boundary)
    rw::WriteVectorOfPOD(sink, e);
}

template <class Source>
void PlaceBoundariesHolder::Locality::Deserialize(Source & src)
{
  m_place = static_cast<LocalityType>(ReadPrimitiveFromSource<int8_t>(src));
  m_placeFromNode = static_cast<LocalityType>(ReadPrimitiveFromSource<int8_t>(src));
  m_population = ReadVarUint<uint64_t>(src);
  m_populationFromNode = ReadVarUint<uint64_t>(src);
  m_adminLevel = ReadPrimitiveFromSource<uint8_t>(src);

  rw::ReadPOD(src, m_center);

  // Same as FeatureBuilder::DeserializeAccuratelyFromIntermediate.
  m_boundary.resize(ReadVarUint<uint32_t>(src));
  for (auto & e : m_boundary)
    rw::ReadVectorOfPOD(src, e);

  RecalcBoundaryRect();

  CHECK(TestValid(), ());
}

bool PlaceBoundariesHolder::Locality::IsBetterBoundary(Locality const & rhs, std::string const & placeName) const
{
  // This function is called for boundary relations only.
  CHECK(!m_boundary.empty() && !rhs.m_boundary.empty(), ());

  if (IsHonestCity() && !rhs.IsHonestCity())
    return true;
  if (!IsHonestCity() && rhs.IsHonestCity())
    return false;

  if (!placeName.empty())
  {
    if (m_name == placeName && rhs.m_name != placeName)
      return true;
    if (m_name != placeName && rhs.m_name == placeName)
      return false;
  }

  // Heuristic: Select boundary with bigger admin_level or smaller boundary.
  if (m_adminLevel == rhs.m_adminLevel)
    return GetApproxArea() < rhs.GetApproxArea();
  return m_adminLevel > rhs.m_adminLevel;
}

double PlaceBoundariesHolder::Locality::GetApproxArea() const
{
  double res = 0;
  for (auto const & poly : m_boundary)
  {
    m2::RectD r;
    CalcRect(poly, r);
    res += r.Area();
  }
  return res;
}

ftypes::LocalityType PlaceBoundariesHolder::Locality::GetPlace() const
{
  return (m_placeFromNode != ftypes::LocalityType::None ? m_placeFromNode : m_place);
}

uint64_t PlaceBoundariesHolder::Locality::GetPopulation() const
{
  uint64_t p = m_populationFromNode;
  if (p == 0)
    p = m_population;

  // Same as ftypes::GetPopulation.
  return (p < 10 ? ftypes::GetDefPopulation(GetPlace()) : p);
}

void PlaceBoundariesHolder::Locality::AssignNodeParams(Locality const & node)
{
  m_center = node.m_center;
  m_placeFromNode = node.m_place;
  m_populationFromNode = node.m_population;
}

bool PlaceBoundariesHolder::Locality::TestValid() const
{
  if (IsPoint())
    return IsHonestCity() && !m_center.IsAlmostZero();

  if (m_boundary.empty() || !m_boundaryRect.IsValid())
    return false;

  if (!IsHonestCity())
  {
    // This Locality is from boundary relation, based on some Node.
    return !m_center.IsAlmostZero() && m_placeFromNode >= ftypes::LocalityType::City && m_adminLevel > 0;
  }

  return true;
}

bool PlaceBoundariesHolder::Locality::IsInBoundary(m2::PointD const & pt) const
{
  CHECK(!m_boundary.empty(), ());
  return m_boundaryRect.IsPointInside(pt);
}

void PlaceBoundariesHolder::Locality::RecalcBoundaryRect()
{
  m_boundaryRect.MakeEmpty();
  for (auto const & pts : m_boundary)
    feature::CalcRect(pts, m_boundaryRect);
}

std::string DebugPrint(PlaceBoundariesHolder::Locality const & l)
{
  std::string rectStr;
  if (l.m_boundaryRect.IsValid())
    rectStr = DebugPrint(mercator::ToLatLon(l.m_boundaryRect));
  else
    rectStr = "Invalid";

  return "Locality { Rect = " + rectStr + "; Name = " + l.m_name + " }";
}

// PlaceBoundariesHolder --------------------------------------------------------------------------

void PlaceBoundariesHolder::Serialize(std::string const & fileName) const
{
  FileWriter writer(fileName);

  WriteVarUint(writer, static_cast<uint64_t>(m_id2index.size()));
  for (auto const & e : m_id2index)
  {
    WriteToSink(writer, e.first.GetEncodedId());
    WriteVarUint(writer, e.second);
  }

  WriteVarUint(writer, static_cast<uint64_t>(m_data.size()));
  for (auto const & e : m_data)
    e.Serialize(writer);
}

void PlaceBoundariesHolder::Deserialize(std::string const & fileName)
{
  FileReader reader(fileName);
  ReaderSource src(reader);

  uint64_t count = ReadVarUint<uint64_t>(src);
  while (count > 0)
  {
    IDType const id(ReadPrimitiveFromSource<uint64_t>(src));
    CHECK(m_id2index.emplace(id, ReadVarUint<uint64_t>(src)).second, (id));
    --count;
  }

  count = ReadVarUint<uint64_t>(src);
  m_data.resize(count);
  for (auto & e : m_data)
    e.Deserialize(src);
}

void PlaceBoundariesHolder::Add(IDType id, Locality && loc, IDType nodeID)
{
  auto [it, added] = m_id2index.emplace(id, m_data.size());
  if (added)
  {
    CHECK(loc.TestValid(), (id));
    m_data.push_back(std::move(loc));
  }

  if (nodeID != IDType())
    CHECK(m_id2index.emplace(nodeID, it->second).second, (id));
}

int PlaceBoundariesHolder::GetIndex(IDType id) const
{
  auto it = m_id2index.find(id);
  if (it != m_id2index.end())
    return it->second;

  LOG(LWARNING, ("Place without locality:", id));
  return -1;
}

PlaceBoundariesHolder::Locality const * PlaceBoundariesHolder::GetBestBoundary(std::vector<IDType> const & ids,
                                                                               m2::PointD const & center) const
{
  Locality const * bestLoc = nullptr;

  auto const isBetter = [&bestLoc](Locality const & loc)
  {
    if (loc.m_boundary.empty())
      return false;
    if (bestLoc == nullptr)
      return true;

    return loc.IsBetterBoundary(*bestLoc);
  };

  for (auto id : ids)
  {
    int const idx = GetIndex(id);
    if (idx < 0)
      continue;

    Locality const & loc = m_data[idx];
    if (isBetter(loc) && loc.IsInBoundary(center))
      bestLoc = &loc;
  }

  return bestLoc;
}

// PlaceBoundariesBuilder -------------------------------------------------------------------------

void PlaceBoundariesBuilder::Add(Locality && loc, IDType id, std::vector<uint64_t> const & nodes)
{
  // Heuristic: store Relation by name only if "connection" nodes are empty.
  // See Place_CityRelations_IncludePoint.
  if (nodes.empty() && !loc.m_name.empty() && id.GetType() == base::GeoObjectId::Type::ObsoleteOsmRelation)
    m_name2rel[loc.m_name].insert(id);

  CHECK(m_id2loc.emplace(id, std::move(loc)).second, ());

  for (uint64_t nodeID : nodes)
    m_node2rel[base::MakeOsmNode(nodeID)].insert(id);
}

/// @todo Make move logic in MergeInto functions?
void PlaceBoundariesBuilder::MergeInto(PlaceBoundariesBuilder & dest) const
{
  for (auto const & e : m_id2loc)
    dest.m_id2loc.emplace(e.first, e.second);

  for (auto const & e : m_node2rel)
    dest.m_node2rel[e.first].insert(e.second.begin(), e.second.end());

  for (auto const & e : m_name2rel)
    dest.m_name2rel[e.first].insert(e.second.begin(), e.second.end());
}

void PlaceBoundariesBuilder::Save(std::string const & fileName)
{
  // Find best Locality Relation for Node.
  std::vector<std::pair<IDType, IDType>> node2rel;

  for (auto const & e : m_id2loc)
  {
    // Iterate via honest Node place localities to select best Relation for them.
    if (!e.second.IsPoint())
      continue;

    IDsSetT ids;
    if (auto it = m_node2rel.find(e.first); it != m_node2rel.end())
      ids.insert(it->second.begin(), it->second.end());
    if (auto it = m_name2rel.find(e.second.m_name); it != m_name2rel.end())
      ids.insert(it->second.begin(), it->second.end());
    if (ids.empty())
      continue;

    CHECK(e.second.IsHonestCity(), (e.first));

    Locality const * best = nullptr;
    IDType bestID;
    for (auto const & relID : ids)
    {
      auto const & loc = m_id2loc[relID];
      if ((best == nullptr || loc.IsBetterBoundary(*best, e.second.m_name)) && loc.IsInBoundary(e.second.m_center))
      {
        best = &loc;
        bestID = relID;
      }
    }

    if (best)
      node2rel.emplace_back(e.first, bestID);
  }

  PlaceBoundariesHolder holder;

  // Add Relation localities with Node refs.
  for (auto const & e : node2rel)
  {
    auto itRelation = m_id2loc.find(e.second);
    CHECK(itRelation != m_id2loc.end(), (e.second));

    auto itNode = m_id2loc.find(e.first);
    CHECK(itNode != m_id2loc.end(), (e.first));

    /// @todo Node params will be assigned form the "first" valid Node now. Implement Update?
    itRelation->second.AssignNodeParams(itNode->second);
    holder.Add(e.second, std::move(itRelation->second), e.first);

    // Do not delete Relation from m_id2loc, because some other Node also can point on it (and we will add reference),
    // but zero this Locality after move to avoid adding below.
    itRelation->second = {};
    CHECK(!itRelation->second.IsHonestCity(), ());

    m_id2loc.erase(itNode);
  }

  // Add remaining localities.
  for (auto & e : m_id2loc)
    if (e.second.IsHonestCity())
      holder.Add(e.first, std::move(e.second), IDType());

  holder.Serialize(fileName);
}

// RoutingCityBoundariesCollector ------------------------------------------------------------------

RoutingCityBoundariesCollector::RoutingCityBoundariesCollector(std::string const & filename,
                                                               IDRInterfacePtr const & cache)
  : CollectorInterface(filename)
  , m_cache(cache)
  , m_featureMakerSimple(cache)
{}

std::shared_ptr<CollectorInterface> RoutingCityBoundariesCollector::Clone(IDRInterfacePtr const & cache) const
{
  return std::make_shared<RoutingCityBoundariesCollector>(GetFilename(), cache ? cache : m_cache);
}

void RoutingCityBoundariesCollector::Collect(OsmElement const & elem)
{
  bool const isRelation = elem.IsRelation();

  // Fast check: is it a place or a boundary candidate.
  std::string place = elem.GetTag("place");
  if (place.empty())
    place = elem.GetTag("de:place");
  if (place.empty() && !isRelation)
    return;

  Locality loc(place, elem);

  std::vector<uint64_t> nodes;
  if (isRelation)
    nodes = osm_element::GetPlaceNodeFromMembers(elem);

  if (!loc.IsHonestCity())
  {
    if (!isRelation)
      return;

    // Accumulate boundaries, where relations doesn't have exact place=city/town/.. tags, like:
    // * Turkey "major" cities have admin_level=4 (like state) with "subtowns" with admin_level=6.
    //    - Istanbul (Kadikoy, Fatih, Beyoglu, ...)
    //    - Ankara (Altindag, ...)
    //    - Izmir
    // * Capitals (like Минск, Москва, Berlin) have admin_level=4.
    // * Canberra's boundary has place=territory
    // * Riviera Beach boundary has place=suburb
    // * Hong Kong boundary has place=region

    if (loc.m_adminLevel < 4)
      return;

    // Skip Relations like boundary=religious_administration.
    // Also "boundary" == "administrative" is missing sometimes.
    auto const boundary = elem.GetTag("boundary");
    if (boundary != "administrative")
      return;

    // Should be at least one reference.
    if (nodes.empty() && loc.m_name.empty())
      return;
  }

  auto copy = elem;
  m_featureMakerSimple.Add(copy);

  bool isGoodFB = false;
  FeatureBuilder fb;
  while (m_featureMakerSimple.GetNextFeature(fb))
  {
    switch (fb.GetGeomType())
    {
    case GeomType::Point: loc.m_center = fb.GetKeyPoint(); break;
    case GeomType::Area:
      /// @todo Move geometry or make parsing geometry without FeatureBuilder class.
      loc.m_boundary.push_back(fb.GetOuterGeometry());
      loc.RecalcBoundaryRect();
      break;

    default:  // skip non-closed ways
      continue;
    }

    isGoodFB = true;
  }

  if (isGoodFB)
    m_builder.Add(std::move(loc), fb.GetMostGenericOsmId(), nodes);
}

void RoutingCityBoundariesCollector::Save()
{
  m_builder.Save(GetFilename());
}

void RoutingCityBoundariesCollector::MergeInto(RoutingCityBoundariesCollector & collector) const
{
  m_builder.MergeInto(collector.m_builder);
}

}  // namespace generator

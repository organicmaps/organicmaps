#include "generator/transit_generator.hpp"

#include "generator/borders_generator.hpp"
#include "generator/borders_loader.hpp"
#include "generator/utils.hpp"

#include "traffic/traffic_cache.hpp"

#include "routing/index_router.hpp"
#include "routing/routing_exceptions.hpp"

#include "routing_common/transit_serdes.hpp"
#include "routing_common/transit_speed_limits.hpp"

#include "storage/country_info_getter.hpp"
#include "storage/routing_helpers.hpp"

#include "indexer/index.hpp"

#include "geometry/mercator.hpp"
#include "geometry/region2d.hpp"

#include "coding/file_container.hpp"
#include "coding/file_name_utils.hpp"
#include "coding/file_writer.hpp"

#include "platform/country_file.hpp"
#include "platform/local_country_file.hpp"
#include "platform/local_country_file_utils.hpp"
#include "platform/platform.hpp"

#include "base/assert.hpp"
#include "base/checked_cast.hpp"
#include "base/logging.hpp"
#include "base/macros.hpp"
#include "base/stl_helpers.hpp"
#include "base/string_utils.hpp"

#include <algorithm>
#include <functional>
#include <iterator>
#include <memory>
#include <utility>

#include "3party/jansson/src/jansson.h"

using namespace generator;
using namespace platform;
using namespace routing;
using namespace routing::transit;
using namespace std;
using namespace storage;

namespace
{
struct ClearVisitor
{
  template <typename Cont>
  void operator()(Cont & c, char const * /* name */) const { c.clear(); }
};

struct SortVisitor
{
  template <typename Cont>
  void operator()(Cont & c, char const * /* name */) const
  {
    sort(c.begin(), c.end());
  }
};

struct IsValidVisitor
{
  template <typename Cont>
  void operator()(Cont const & c, char const * /* name */ )
  {
    m_isValid = m_isValid && ::IsValid(c);
  }

  bool IsValid() const { return m_isValid; }

private:
  bool m_isValid = true;
};

struct IsUniqueVisitor
{
  template <typename Cont>
  void operator()(Cont const & c, char const * /* name */)
  {
    m_isUnique = m_isUnique && (adjacent_find(c.cbegin(), c.cend()) == c.cend());
  }

  bool IsUnique() const { return m_isUnique; }

private:
  bool m_isUnique = true;
};

struct IsSortedVisitor
{
  template <typename Cont>
  void operator()(Cont const & c, char const * /* name */)
  {
    m_isSorted = m_isSorted && is_sorted(c.cbegin(), c.cend());
  }

  bool IsSorted() const { return m_isSorted; }

private:
  bool m_isSorted = true;
};

template <typename T>
void Append(vector<T> const & src, vector<T> & dst)
{
  dst.insert(dst.end(), src.begin(), src.end());
}

void LoadBorders(string const & dir, TCountryId const & countryId, vector<m2::RegionD> & borders)
{
  string const polyFile = my::JoinPath(dir, BORDERS_DIR, countryId + BORDERS_EXTENSION);
  borders.clear();
  osm::LoadBorders(polyFile, borders);
}

bool HasStop(vector<Stop> const & stops, StopId stopId)
{
  return binary_search(stops.cbegin(), stops.cend(), Stop(stopId));
}

/// \brief Removes from |items| all items which stop ids is not contained in |stops|.
/// \note This method keeps relative order of |items|.
template <class Item>
void ClipItemsByStops(vector<Stop> const & stops, vector<Item> & items)
{
  CHECK(is_sorted(stops.cbegin(), stops.cend()), ());

  vector<Item> itemsToFill;
  for (auto const & item : items)
  {
    for (auto const stopId : item.GetStopIds())
    {
      if (HasStop(stops, stopId))
      {
        itemsToFill.push_back(item);
        break;
      }
    }
  }

  items.swap(itemsToFill);
}

/// \returns ref to an item at |items| by |id|.
/// \note |items| must be sorted before a call of this method.
template <class Id, class Item>
Item const & FindById(vector<Item> const & items, Id id)
{
  auto const s1Id = equal_range(items.cbegin(), items.cend(), Item(id));
  CHECK_EQUAL(distance(s1Id.first, s1Id.second), 1,
              ("An item with id:", id, "is not unique or there's not such item. items:", items));
  return *s1Id.first;
}

/// \brief Fills |items| with items which have ids from |ids|.
/// \note |items| must be sorted before a call of this method.
template <class Id, class Item>
void UpdateItems(set<Id> const & ids, vector<Item> & items)
{
  vector<Item> itemsToFill;
  for (auto const id : ids)
    itemsToFill.push_back(FindById(items, id));

  SortVisitor{}(itemsToFill, nullptr /* name */);
  items.swap(itemsToFill);
}

void FillOsmIdToFeatureIdsMap(string const & osmIdToFeatureIdsPath, OsmIdToFeatureIdsMap & map)
{
  CHECK(ForEachOsmId2FeatureId(osmIdToFeatureIdsPath,
                               [&map](osm::Id const & osmId, uint32_t featureId) {
                                 map[osmId].push_back(featureId);
                               }),
        (osmIdToFeatureIdsPath));
}

string GetMwmPath(string const & mwmDir, TCountryId const & countryId)
{
  return my::JoinPath(mwmDir, countryId + DATA_FILE_EXTENSION);
}

}  // namespace

namespace routing
{
namespace transit
{
// DeserializerFromJson ---------------------------------------------------------------------------
DeserializerFromJson::DeserializerFromJson(json_struct_t * node,
                                           OsmIdToFeatureIdsMap const & osmIdToFeatureIds)
    : m_node(node), m_osmIdToFeatureIds(osmIdToFeatureIds)
{
}

void DeserializerFromJson::operator()(m2::PointD & p, char const * name)
{
  // @todo(bykoianko) Instead of having a special operator() method for m2::PointD class it's necessary to
  // add Point class to transit_types.hpp and process it in DeserializerFromJson with regular method.
  json_t * item = nullptr;
  if (name == nullptr)
    item = m_node; // Array item case
  else
    item = my::GetJSONObligatoryField(m_node, name);

  CHECK(json_is_object(item), ("Item is not a json object:", name));
  FromJSONObject(item, "x", p.x);
  FromJSONObject(item, "y", p.y);
}

void DeserializerFromJson::operator()(FeatureIdentifiers & id, char const * name)
{
  // Conversion osm id to feature id.
  string osmIdStr;
  GetField(osmIdStr, name);
  CHECK(strings::is_number(osmIdStr), ("Osm id string is not a number:", osmIdStr));
  uint64_t osmIdNum;
  CHECK(strings::to_uint64(osmIdStr, osmIdNum),
        ("Cann't convert osm id string:", osmIdStr, "to a number."));
  osm::Id const osmId(osmIdNum);
  auto const it = m_osmIdToFeatureIds.find(osmId);
  if (it != m_osmIdToFeatureIds.cend())
  {
    CHECK_GREATER_OR_EQUAL(it->second.size(), 1, ("Osm id:", osmId, "(encoded", osmId.EncodedId(),
                            ") from transit graph corresponds to zero features."));
    if (it->second.size() == 1)
    {
      id.SetFeatureId(it->second[0]);
    }
    else
    {
      LOG(LWARNING, ("Osm id:", osmId, "(encoded", osmId.EncodedId(), "corresponds to",
                     it->second.size(), "feature ids. It may happen in rare case."));
      // Note. |osmId| corresponds several feature ids. It may happen in case of stops;
      // if a stop is present as relation.
      id.SetFeatureId(it->second[0]);
    }
  }
  id.SetOsmId(osmId.EncodedId());
}

void DeserializerFromJson::operator()(EdgeFlags & edgeFlags, char const * name)
{
  bool transfer = false;
  (*this)(transfer, name);
  // Note. Only |transfer| field of |edgeFlags| may be set at this point because the
  // other fields of |edgeFlags| are unknown.
  edgeFlags.SetFlags(0);
  edgeFlags.m_transfer = transfer;
}

void DeserializerFromJson::operator()(StopIdRanges & rs, char const * name)
{
  vector<StopId> stopIds;
  (*this)(stopIds, name);
  rs = StopIdRanges({stopIds});
}

// GraphData --------------------------------------------------------------------------------------
void GraphData::DeserializeFromJson(my::Json const & root, OsmIdToFeatureIdsMap const & mapping)
{
  DeserializerFromJson deserializer(root.get(), mapping);
  Visit(deserializer);

  // Removes equivalent edges from |m_edges|. If there are several equivalent edges only
  // the most lightweight edge is left.
  // Note. It's possible that two stops are connected with the same line several times
  // in the same direction. It happens in Oslo metro (T-banen):
  // https://en.wikipedia.org/wiki/Oslo_Metro#/media/File:Oslo_Metro_Map.svg branch 5.
  my::SortUnique(m_edges,
                 [](Edge const & e1, Edge const & e2) {
                   if (e1 != e2)
                     return e1 < e2;
                   return e1.GetWeight() < e2.GetWeight();
                 },
                 [](Edge const & e1, Edge const & e2) { return e1 == e2; });
}

void GraphData::Serialize(Writer & writer) const
{
  TransitHeader header;

  auto const startOffset = writer.Pos();
  Serializer<Writer> serializer(writer);
  FixedSizeSerializer<Writer> numberSerializer(writer);
  numberSerializer(header);
  header.m_stopsOffset = base::checked_cast<uint32_t>(writer.Pos() - startOffset);

  serializer(m_stops);
  header.m_gatesOffset = base::checked_cast<uint32_t>(writer.Pos() - startOffset);

  serializer(m_gates);
  header.m_edgesOffset = base::checked_cast<uint32_t>(writer.Pos() - startOffset);

  serializer(m_edges);
  header.m_transfersOffset = base::checked_cast<uint32_t>(writer.Pos() - startOffset);

  serializer(m_transfers);
  header.m_linesOffset = base::checked_cast<uint32_t>(writer.Pos() - startOffset);

  serializer(m_lines);
  header.m_shapesOffset = base::checked_cast<uint32_t>(writer.Pos() - startOffset);

  serializer(m_shapes);
  header.m_networksOffset = base::checked_cast<uint32_t>(writer.Pos() - startOffset);

  serializer(m_networks);
  header.m_endOffset = base::checked_cast<uint32_t>(writer.Pos() - startOffset);

  // Rewriting header info.
  CHECK(header.IsValid(), (header));
  auto const endOffset = writer.Pos();
  writer.Seek(startOffset);
  numberSerializer(header);
  writer.Seek(endOffset);

  LOG(LINFO, (TRANSIT_FILE_TAG, "section is ready. Header:", header));
}

void GraphData::Deserialize(Reader & reader)
{
  NonOwningReaderSource src(reader);
  transit::Deserializer<NonOwningReaderSource> deserializer(src);
  transit::FixedSizeDeserializer<NonOwningReaderSource> numberDeserializer(src);

  transit::TransitHeader header;
  numberDeserializer(header);
  CHECK(header.IsValid(), ());

  CHECK_EQUAL(src.Pos(), header.m_stopsOffset, ("Wrong", TRANSIT_FILE_TAG, "section format."));
  deserializer(m_stops);
  CHECK(transit::IsValidSortedUnique(m_stops), ());

  CHECK_EQUAL(src.Pos(), header.m_gatesOffset, ("Wrong", TRANSIT_FILE_TAG, "section format."));
  deserializer(m_gates);
  CHECK(transit::IsValidSortedUnique(m_gates), ());

  CHECK_EQUAL(src.Pos(), header.m_edgesOffset, ("Wrong", TRANSIT_FILE_TAG, "section format."));
  deserializer(m_edges);
  CHECK(transit::IsValidSortedUnique(m_edges), ());

  CHECK_EQUAL(src.Pos(), header.m_transfersOffset, ("Wrong", TRANSIT_FILE_TAG, "section format."));
  deserializer(m_transfers);
  CHECK(transit::IsValidSortedUnique(m_transfers), ());

  CHECK_EQUAL(src.Pos(), header.m_linesOffset, ("Wrong", TRANSIT_FILE_TAG, "section format."));
  deserializer(m_lines);
  CHECK(transit::IsValidSortedUnique(m_lines), ());

  CHECK_EQUAL(src.Pos(), header.m_shapesOffset, ("Wrong", TRANSIT_FILE_TAG, "section format."));
  deserializer(m_shapes);
  CHECK(transit::IsValidSortedUnique(m_shapes), ());

  CHECK_EQUAL(src.Pos(), header.m_networksOffset, ("Wrong", TRANSIT_FILE_TAG, "section format."));
  deserializer(m_networks);
  CHECK(transit::IsValidSortedUnique(m_networks), ());

  CHECK_EQUAL(src.Pos(), header.m_endOffset, ("Wrong", TRANSIT_FILE_TAG, "section format."));
}

void GraphData::AppendTo(GraphData const & rhs)
{
  ::Append(rhs.m_stops, m_stops);
  ::Append(rhs.m_gates, m_gates);
  ::Append(rhs.m_edges, m_edges);
  ::Append(rhs.m_transfers, m_transfers);
  ::Append(rhs.m_lines, m_lines);
  ::Append(rhs.m_shapes, m_shapes);
  ::Append(rhs.m_networks, m_networks);
}

void GraphData::Clear()
{
  ClearVisitor const v{};
  Visit(v);
}

bool GraphData::IsValid() const
{
  if (!IsSorted() || !IsUnique())
    return false;

  IsValidVisitor v;
  Visit(v);
  return v.IsValid();
}

bool GraphData::IsEmpty() const
{
  // Note. |m_transfers| may be empty if GraphData instance is not empty.
  return m_stops.empty() || m_gates.empty() || m_edges.empty() || m_lines.empty()
      || m_shapes.empty() || m_networks.empty();
}

void GraphData::Sort()
{
  SortVisitor const v{};
  Visit(v);
}

void GraphData::ClipGraph(vector<m2::RegionD> const & borders)
{
  Sort();
  CHECK(IsValid(), ());
  ClipLines(borders);
  ClipStops();
  ClipNetworks();
  ClipGates();
  ClipTransfer();
  ClipEdges();
  ClipShapes();
  CHECK(IsValid(), ());
}

void GraphData::CalculateBestPedestrianSegments(string const & mwmPath, TCountryId const & countryId)
{
  // Creating IndexRouter.
  SingleMwmIndex index(mwmPath);

  auto infoGetter = storage::CountryInfoReader::CreateCountryInfoReader(GetPlatform());
  CHECK(infoGetter, ());

  auto const countryFileGetter = [&infoGetter](m2::PointD const & pt) {
    return infoGetter->GetRegionCountryId(pt);
  };

  auto const getMwmRectByName = [&](string const & c) -> m2::RectD {
    CHECK_EQUAL(countryId, c, ());
    return infoGetter->GetLimitRectForLeaf(c);
  };

  CHECK_EQUAL(index.GetMwmId().GetInfo()->GetType(), MwmInfo::COUNTRY, ());
  auto numMwmIds = make_shared<NumMwmIds>();
  numMwmIds->RegisterFile(CountryFile(countryId));

  // Note. |indexRouter| is valid while |index| is valid.
  IndexRouter indexRouter(VehicleType::Pedestrian, false /* load altitudes */,
                          CountryParentNameGetterFn(), countryFileGetter, getMwmRectByName,
                          numMwmIds, MakeNumMwmTree(*numMwmIds, *infoGetter),
                          traffic::TrafficCache(), index.GetIndex());
  auto worldGraph = indexRouter.MakeSingleMwmWorldGraph();

  // Looking for the best segment for every gate.
  for (auto & gate : m_gates)
  {
    if (countryFileGetter(gate.GetPoint()) != countryId)
      continue;
    // Note. For pedestrian routing all the segments are considered as twoway segments so
    // IndexRouter.FindBestSegment() method finds the same segment for |isOutgoing| == true
    // and |isOutgoing| == false.
    Segment bestSegment;
    try
    {
      if (countryFileGetter(gate.GetPoint()) != countryId)
        continue;
      if (indexRouter.FindBestSegment(gate.GetPoint(), m2::PointD::Zero() /* direction */,
                                      true /* isOutgoing */, *worldGraph, bestSegment))
      {
        CHECK_EQUAL(bestSegment.GetMwmId(), 0, ());
        gate.SetBestPedestrianSegment(SingleMwmSegment(
            bestSegment.GetFeatureId(), bestSegment.GetSegmentIdx(), bestSegment.IsForward()));
      }
    }
    catch (MwmIsNotAliveException const & e)
    {
      LOG(LCRITICAL, ("Point of a gate belongs to the processed mwm:", countryId, ","
          "but the mwm is not alive. Gate:", gate, e.what()));
    }
    catch (RootException const & e)
    {
      LOG(LCRITICAL, ("Exception while looking for the best segment of a gate. CountryId:",
          countryId, ". Gate:", gate, e.what()));
    }
  }
}

bool GraphData::IsUnique() const
{
  IsUniqueVisitor v;
  Visit(v);
  return v.IsUnique();
}

bool GraphData::IsSorted() const
{
  IsSortedVisitor v;
  Visit(v);
  return v.IsSorted();
}

void GraphData::ClipLines(vector<m2::RegionD> const & borders)
{
  // Set with stop ids with stops which are inside |borders|.
  set<StopId> stopIdInside;
  for (auto const & stop : m_stops)
  {
    if (m2::RegionsContain(borders, stop.GetPoint()))
      stopIdInside.insert(stop.GetId());
  }

  set<StopId> hasNeighborInside;
  for (auto const & edge : m_edges)
  {
    auto const stop1Inside = stopIdInside.count(edge.GetStop1Id()) != 0;
    auto const stop2Inside = stopIdInside.count(edge.GetStop2Id()) != 0;
    if (stop1Inside && !stop2Inside)
      hasNeighborInside.insert(edge.GetStop2Id());
    if (stop2Inside && !stop1Inside)
      hasNeighborInside.insert(edge.GetStop1Id());
  }

  stopIdInside.insert(hasNeighborInside.cbegin(), hasNeighborInside.cend());

  // Filling |lines| with stops inside |borders|.
  vector<Line> lines;
  for (auto const & line : m_lines)
  {
    // Note. |stopIdsToFill| will be filled with continuous sequences of stop ids.
    // In most cases only one sequence of stop ids should be placed to |stopIdsToFill|.
    // But if a line is split by |borders| several times then several
    // continuous groups of stop ids will be placed to |stopIdsToFill|.
    // The loop below goes through all the stop ids belong the line |line| and
    // keeps in |stopIdsToFill| continuous groups of stop ids which are inside |borders|.
    Ranges stopIdsToFill;
    Ranges const & ranges = line.GetStopIds();
    CHECK_EQUAL(ranges.size(), 1, ());
    vector<StopId> const & stopIds = ranges[0];
    auto it = stopIds.begin();
    while (it != stopIds.end()) {
      while (it != stopIds.end() && stopIdInside.count(*it) == 0)
        ++it;
      auto jt = it;
      while (jt != stopIds.end() && stopIdInside.count(*jt) != 0)
        ++jt;
      if (it != jt)
        stopIdsToFill.emplace_back(it, jt);
      it = jt;
    }

    if (!stopIdsToFill.empty())
    {
      lines.emplace_back(line.GetId(), line.GetNumber(), line.GetTitle(), line.GetType(),
                         line.GetColor(), line.GetNetworkId(), stopIdsToFill, line.GetInterval());
    }
  }

  m_lines.swap(lines);
}

void GraphData::ClipStops()
{
  CHECK(is_sorted(m_stops.cbegin(), m_stops.cend()), ());
  set<StopId> stopIds;
  for (auto const & line : m_lines)
  {
    for (auto const & range : line.GetStopIds())
      stopIds.insert(range.cbegin(), range.cend());
  }

  UpdateItems(stopIds, m_stops);
}

void GraphData::ClipNetworks()
{
  CHECK(is_sorted(m_networks.cbegin(), m_networks.cend()), ());
  set<NetworkId> networkIds;
  for (auto const & line : m_lines)
    networkIds.insert(line.GetNetworkId());

  UpdateItems(networkIds, m_networks);
}

void GraphData::ClipGates()
{
  ClipItemsByStops(m_stops, m_gates);
}

void GraphData::ClipTransfer()
{
  ClipItemsByStops(m_stops, m_transfers);
}

void GraphData::ClipEdges()
{
  CHECK(is_sorted(m_stops.cbegin(), m_stops.cend()), ());

  vector<Edge> edges;
  for (auto const & edge : m_edges)
  {
    if (HasStop(m_stops, edge.GetStop1Id()) && HasStop(m_stops, edge.GetStop2Id()))
      edges.push_back(edge);
  }

  SortVisitor{}(edges, nullptr /* name */);
  m_edges.swap(edges);
}

void GraphData::ClipShapes()
{
  CHECK(is_sorted(m_edges.cbegin(), m_edges.cend()), ());

  // Set with shape ids contained in m_edges.
  set<ShapeId> shapeIdInEdges;
  for (auto const & edge : m_edges)
  {
    auto const & shapeIds = edge.GetShapeIds();
    shapeIdInEdges.insert(shapeIds.cbegin(), shapeIds.cend());
  }

  vector<Shape> shapes;
  for (auto const & shape : m_shapes)
  {
    if (shapeIdInEdges.count(shape.GetId()) != 0)
      shapes.push_back(shape);
  }

  m_shapes.swap(shapes);
}

void DeserializeFromJson(OsmIdToFeatureIdsMap const & mapping,
                         string const & transitJsonPath, GraphData & data)
{
  Platform::EFileType fileType;
  Platform::EError const errCode = Platform::GetFileType(transitJsonPath, fileType);
  CHECK_EQUAL(errCode, Platform::EError::ERR_OK, ("Transit graph was not found:", transitJsonPath));
  CHECK_EQUAL(fileType, Platform::EFileType::FILE_TYPE_REGULAR,
              ("Transit graph was not found:", transitJsonPath));

  string jsonBuffer;
  try
  {
    GetPlatform().GetReader(transitJsonPath)->ReadAsString(jsonBuffer);
  }
  catch (RootException const & e)
  {
    LOG(LCRITICAL, ("Can't open", transitJsonPath, e.what()));
  }

  my::Json root(jsonBuffer.c_str());
  CHECK(root.get() != nullptr, ("Cannot parse the json file:", transitJsonPath));

  data.Clear();
  try
  {
    data.DeserializeFromJson(root, mapping);
  }
  catch (RootException const & e)
  {
    LOG(LCRITICAL, ("Exception while parsing transit graph json. Json file path:", transitJsonPath, e.what()));
  }
}

void ProcessGraph(string const & mwmPath, TCountryId const & countryId,
                  OsmIdToFeatureIdsMap const & osmIdToFeatureIdsMap, GraphData & data)
{
  data.CalculateBestPedestrianSegments(mwmPath, countryId);
  data.Sort();
  CHECK(data.IsValid(), (mwmPath));
}

void BuildTransit(string const & mwmDir, TCountryId const & countryId,
                  string const & osmIdToFeatureIdsPath, string const & transitDir)
{
  LOG(LINFO, ("Building transit section for", countryId));
  Platform::FilesList graphFiles;
  Platform::GetFilesByExt(my::AddSlashIfNeeded(transitDir), TRANSIT_FILE_EXTENSION, graphFiles);

  string const mwmPath = GetMwmPath(mwmDir, countryId);
  OsmIdToFeatureIdsMap mapping;
  FillOsmIdToFeatureIdsMap(osmIdToFeatureIdsPath, mapping);
  vector<m2::RegionD> mwmBorders;
  LoadBorders(mwmDir, countryId, mwmBorders);

  GraphData jointData;
  for (auto const & fileName : graphFiles)
  {
    auto const filePath = my::JoinPath(transitDir, fileName);
    GraphData data;
    DeserializeFromJson(mapping, filePath, data);
    // @todo(bykoianko) Json should be clipped on feature generation step. It's much more efficient.
    data.ClipGraph(mwmBorders);
    jointData.AppendTo(data);
  }

  if (jointData.IsEmpty())
    return; // Empty transit section.

  ProcessGraph(mwmPath, countryId, mapping, jointData);
  CHECK(jointData.IsValid(), (mwmPath, countryId));

  FilesContainerW cont(mwmPath, FileWriter::OP_WRITE_EXISTING);
  FileWriter writer = cont.GetWriter(TRANSIT_FILE_TAG);
  jointData.Serialize(writer);
}
}  // namespace transit
}  // namespace routing

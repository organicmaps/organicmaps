#include "transit/transit_graph_data.hpp"

#include "transit/transit_serdes.hpp"

#include "base/assert.hpp"
#include "base/checked_cast.hpp"
#include "base/logging.hpp"
#include "base/stl_helpers.hpp"
#include "base/string_utils.hpp"

#include <algorithm>
#include <iterator>
#include <set>

#include "defines.hpp"

namespace routing
{
namespace transit
{
using namespace routing;
using namespace std;

namespace
{
struct ClearVisitor
{
  template <typename Cont>
  void operator()(Cont & c, char const * /* name */) const
  {
    c.clear();
  }
};

struct SortVisitor
{
  template <typename Cont>
  void operator()(Cont & c, char const * /* name */) const
  {
    sort(c.begin(), c.end());

    auto const end = c.end();
    auto const it = unique(c.begin(), end, [](auto const & l, auto const & r)
    {
      if (l == r)
      {
        // Print _right_ element as next equal entry.
        LOG(LINFO, ("Duplicating entry:", r));
        return true;
      }
      return false;
    });

    if (it != end)
    {
      LOG(LINFO, ("Will be deleted", std::distance(it, end), "elements"));
      c.erase(it, end);
    }
  }
};

struct CheckValidVisitor
{
  template <typename Cont>
  void operator()(Cont const & c, char const * name)
  {
    CheckValid(c, name);
  }
};

struct CheckUniqueVisitor
{
  template <typename Cont>
  void operator()(Cont const & c, char const * name)
  {
    CheckUnique(c, name);
  }
};

struct CheckSortedVisitor
{
  template <typename Cont>
  void operator()(Cont const & c, char const * name)
  {
    CheckSorted(c, name);
  }
};

template <typename T>
void Append(vector<T> const & src, vector<T> & dst)
{
  dst.insert(dst.end(), src.begin(), src.end());
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
  itemsToFill.reserve(ids.size());
  for (auto const id : ids)
    itemsToFill.push_back(FindById(items, id));

  SortVisitor{}(itemsToFill, nullptr /* name */);
  items.swap(itemsToFill);
}

template <class Item>
void ReadItems(uint32_t start, uint32_t end, string const & name, NonOwningReaderSource & src, vector<Item> & items)
{
  Deserializer<NonOwningReaderSource> deserializer(src);
  CHECK_EQUAL(src.Pos(), start, ("Wrong", TRANSIT_FILE_TAG, "section format. Table name:", name));
  deserializer(items);
  CHECK_EQUAL(src.Pos(), end, ("Wrong", TRANSIT_FILE_TAG, "section format. Table name:", name));
  CheckValidSortedUnique(items, name);
}
}  // namespace

// DeserializerFromJson ---------------------------------------------------------------------------
DeserializerFromJson::DeserializerFromJson(json_t * node, OsmIdToFeatureIdsMap const & osmIdToFeatureIds)
  : m_node(node)
  , m_osmIdToFeatureIds(osmIdToFeatureIds)
{}

void DeserializerFromJson::operator()(m2::PointD & p, char const * name)
{
  // @todo(bykoianko) Instead of having a special operator() method for m2::PointD class it's
  // necessary to add Point class to transit_types.hpp and process it in DeserializerFromJson with
  // regular method.
  json_t * item = nullptr;
  if (name == nullptr)
    item = m_node;  // Array item case
  else
    item = base::GetJSONObligatoryField(m_node, name);

  CHECK(json_is_object(item), ("Item is not a json object:", name));
  FromJSONObject(item, "x", p.x);
  FromJSONObject(item, "y", p.y);
}

void DeserializerFromJson::operator()(FeatureIdentifiers & id, char const * name)
{
  // Conversion osm id to feature id.
  string osmIdStr;
  GetField(osmIdStr, name);
  uint64_t osmIdNum;
  CHECK(strings::to_uint64(osmIdStr, osmIdNum), ("Cann't convert osm id string:", osmIdStr, "to a number."));
  base::GeoObjectId const osmId(osmIdNum);
  auto const it = m_osmIdToFeatureIds.find(osmId);
  if (it != m_osmIdToFeatureIds.cend())
  {
    CHECK(!it->second.empty(), ("Osm id:", osmId, "(encoded", osmId.GetEncodedId(),
                                ") from transit graph does not correspond to any feature."));
    if (it->second.size() != 1)
    {
      // Note. |osmId| corresponds to several feature ids. It may happen in case of stops,
      // if a stop is present as a relation. It's a rare case.
      LOG(LWARNING,
          ("Osm id:", osmId, "( encoded", osmId.GetEncodedId(), ") corresponds to", it->second.size(), "feature ids."));
    }
    id.SetFeatureId(it->second[0]);
  }
  id.SetOsmId(osmId.GetEncodedId());
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
void GraphData::DeserializeFromJson(base::Json const & root, OsmIdToFeatureIdsMap const & mapping)
{
  DeserializerFromJson deserializer(root.get(), mapping);
  Visit(deserializer);

  // Removes equivalent edges from |m_edges|. If there are several equivalent edges only
  // the most lightweight edge is left.
  // Note. It's possible that two stops are connected with the same line several times
  // in the same direction. It happens in Oslo metro (T-banen):
  // https://en.wikipedia.org/wiki/Oslo_Metro#/media/File:Oslo_Metro_Map.svg branch 5.
  base::SortUnique(m_edges, [](Edge const & e1, Edge const & e2)
  {
    if (e1 != e2)
      return e1 < e2;
    return e1.GetWeight() < e2.GetWeight();
  }, [](Edge const & e1, Edge const & e2) { return e1 == e2; });
}

void GraphData::Serialize(Writer & writer)
{
  auto const startOffset = writer.Pos();
  Serializer<Writer> serializer(writer);
  FixedSizeSerializer<Writer> numberSerializer(writer);
  m_header.Reset();
  numberSerializer(m_header);

  m_header.m_stopsOffset = base::checked_cast<uint32_t>(writer.Pos() - startOffset);
  serializer(m_stops);

  m_header.m_gatesOffset = base::checked_cast<uint32_t>(writer.Pos() - startOffset);
  serializer(m_gates);

  m_header.m_edgesOffset = base::checked_cast<uint32_t>(writer.Pos() - startOffset);
  serializer(m_edges);

  m_header.m_transfersOffset = base::checked_cast<uint32_t>(writer.Pos() - startOffset);
  serializer(m_transfers);

  m_header.m_linesOffset = base::checked_cast<uint32_t>(writer.Pos() - startOffset);
  serializer(m_lines);

  m_header.m_shapesOffset = base::checked_cast<uint32_t>(writer.Pos() - startOffset);
  serializer(m_shapes);

  m_header.m_networksOffset = base::checked_cast<uint32_t>(writer.Pos() - startOffset);
  serializer(m_networks);

  m_header.m_endOffset = base::checked_cast<uint32_t>(writer.Pos() - startOffset);

  // Rewriting header info.
  CHECK(m_header.IsValid(), (m_header));
  auto const endOffset = writer.Pos();
  writer.Seek(startOffset);
  numberSerializer(m_header);
  writer.Seek(endOffset);

  LOG(LINFO, (TRANSIT_FILE_TAG, "section is ready. Header:", m_header));
}

void GraphData::DeserializeAll(Reader & reader)
{
  DeserializeWith(reader, [this](NonOwningReaderSource & src)
  {
    ReadStops(src);
    ReadGates(src);
    ReadEdges(src);
    ReadTransfers(src);
    ReadLines(src);
    ReadShapes(src);
    ReadNetworks(src);
  });
}

void GraphData::DeserializeForRouting(Reader & reader)
{
  DeserializeWith(reader, [this](NonOwningReaderSource & src)
  {
    ReadStops(src);
    ReadGates(src);
    ReadEdges(src);
    src.Skip(m_header.m_linesOffset - src.Pos());
    ReadLines(src);
  });
}

void GraphData::DeserializeForRendering(Reader & reader)
{
  DeserializeWith(reader, [this](NonOwningReaderSource & src)
  {
    ReadStops(src);
    src.Skip(m_header.m_transfersOffset - src.Pos());
    ReadTransfers(src);
    ReadLines(src);
    ReadShapes(src);
  });
}

void GraphData::DeserializeForCrossMwm(Reader & reader)
{
  DeserializeWith(reader, [this](NonOwningReaderSource & src)
  {
    ReadStops(src);
    src.Skip(m_header.m_edgesOffset - src.Pos());
    ReadEdges(src);
  });
}

void GraphData::AppendTo(GraphData const & rhs)
{
  Append(rhs.m_stops, m_stops);
  Append(rhs.m_gates, m_gates);
  Append(rhs.m_edges, m_edges);
  Append(rhs.m_transfers, m_transfers);
  Append(rhs.m_lines, m_lines);
  Append(rhs.m_shapes, m_shapes);
  Append(rhs.m_networks, m_networks);
}

void GraphData::Clear()
{
  ClearVisitor const v{};
  Visit(v);
}

void GraphData::CheckValidSortedUnique() const
{
  {
    CheckSortedVisitor v;
    Visit(v);
  }

  {
    CheckUniqueVisitor v;
    Visit(v);
  }

  {
    CheckValidVisitor v;
    Visit(v);
  }
}

bool GraphData::IsEmpty() const
{
  // Note. |m_transfers| may be empty if GraphData instance is not empty.
  return m_stops.empty() || m_gates.empty() || m_edges.empty() || m_lines.empty() || m_shapes.empty() ||
         m_networks.empty();
}

void GraphData::Sort()
{
  SortVisitor const v{};
  Visit(v);
}

void GraphData::ClipGraph(vector<m2::RegionD> const & borders)
{
  Sort();
  CheckValidSortedUnique();
  ClipLines(borders);
  ClipStops();
  ClipNetworks();
  ClipGates();
  ClipTransfer();
  ClipEdges();
  ClipShapes();
  CheckValidSortedUnique();
}

void GraphData::SetGateBestPedestrianSegment(size_t gateIdx, SingleMwmSegment const & s)
{
  CHECK_LESS(gateIdx, m_gates.size(), ());
  m_gates[gateIdx].SetBestPedestrianSegment(s);
}

void GraphData::ClipLines(vector<m2::RegionD> const & borders)
{
  // Set with stop ids with stops which are inside |borders|.
  set<StopId> stopIdInside;
  for (auto const & stop : m_stops)
    if (m2::RegionsContain(borders, stop.GetPoint()))
      stopIdInside.insert(stop.GetId());

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
    while (it != stopIds.end())
    {
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
      lines.emplace_back(line.GetId(), line.GetNumber(), line.GetTitle(), line.GetType(), line.GetColor(),
                         line.GetNetworkId(), stopIdsToFill, line.GetInterval());
    }
  }

  m_lines.swap(lines);
}

void GraphData::ClipStops()
{
  CHECK(is_sorted(m_stops.cbegin(), m_stops.cend()), ());
  set<StopId> stopIds;
  for (auto const & line : m_lines)
    for (auto const & range : line.GetStopIds())
      stopIds.insert(range.cbegin(), range.cend());

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
    if (HasStop(m_stops, edge.GetStop1Id()) && HasStop(m_stops, edge.GetStop2Id()))
      edges.push_back(edge);

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
    if (shapeIdInEdges.count(shape.GetId()) != 0)
      shapes.push_back(shape);

  m_shapes.swap(shapes);
}

void GraphData::ReadHeader(NonOwningReaderSource & src)
{
  FixedSizeDeserializer<NonOwningReaderSource> numberDeserializer(src);
  numberDeserializer(m_header);
  CHECK_EQUAL(src.Pos(), m_header.m_stopsOffset, ("Wrong", TRANSIT_FILE_TAG, "section format."));
  CHECK(m_header.IsValid(), ());
}

void GraphData::ReadStops(NonOwningReaderSource & src)
{
  ReadItems(m_header.m_stopsOffset, m_header.m_gatesOffset, "stops", src, m_stops);
}

void GraphData::ReadGates(NonOwningReaderSource & src)
{
  ReadItems(m_header.m_gatesOffset, m_header.m_edgesOffset, "gates", src, m_gates);
}

void GraphData::ReadEdges(NonOwningReaderSource & src)
{
  ReadItems(m_header.m_edgesOffset, m_header.m_transfersOffset, "edges", src, m_edges);
}

void GraphData::ReadTransfers(NonOwningReaderSource & src)
{
  ReadItems(m_header.m_transfersOffset, m_header.m_linesOffset, "transfers", src, m_transfers);
}

void GraphData::ReadLines(NonOwningReaderSource & src)
{
  ReadItems(m_header.m_linesOffset, m_header.m_shapesOffset, "lines", src, m_lines);
}

void GraphData::ReadShapes(NonOwningReaderSource & src)
{
  ReadItems(m_header.m_shapesOffset, m_header.m_networksOffset, "shapes", src, m_shapes);
}

void GraphData::ReadNetworks(NonOwningReaderSource & src)
{
  ReadItems(m_header.m_networksOffset, m_header.m_endOffset, "networks", src, m_networks);
}
}  // namespace transit
}  // namespace routing

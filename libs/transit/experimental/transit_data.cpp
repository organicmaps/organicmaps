#include "transit/experimental/transit_data.hpp"

#include "transit/transit_entities.hpp"
#include "transit/transit_serdes.hpp"
#include "transit/transit_version.hpp"

#include "base/assert.hpp"
#include "base/file_name_utils.hpp"
#include "base/logging.hpp"

#include <algorithm>
#include <fstream>
#include <tuple>

namespace transit
{
namespace experimental
{
template <class E>
void ReadItems(uint32_t start, uint32_t end, std::string const & entityName, NonOwningReaderSource & src,
               std::vector<E> & entities)
{
  routing::transit::Deserializer<NonOwningReaderSource> deserializer(src);

  CHECK_EQUAL(src.Pos(), start, ("Wrong", TRANSIT_FILE_TAG, "section format for:", entityName));
  deserializer(entities);
  CHECK_EQUAL(src.Pos(), end, ("Wrong", TRANSIT_FILE_TAG, "section format for", entityName));
}

struct ClearVisitor
{
  template <class C>
  void operator()(C & container, char const * /* entityName */) const
  {
    container.clear();
  }
};

struct SortVisitor
{
  template <class C>
  void operator()(C & container, char const * /* entityName */) const
  {
    std::sort(container.begin(), container.end());
  }
};

struct CheckValidVisitor
{
  template <class C>
  void operator()(C const & container, char const * entityName) const
  {
    for (auto const & item : container)
      CHECK(item.IsValid(), (item, "is not valid in", entityName));
  }
};

struct CheckUniqueVisitor
{
  template <class C>
  void operator()(C const & container, char const * entityName) const
  {
    auto const it = std::adjacent_find(container.begin(), container.end());
    CHECK(it == container.end(), (*it, "is not unique in", entityName));
  }
};

struct CheckSortedVisitor
{
  template <typename C>
  void operator()(C const & container, char const * entityName) const
  {
    CHECK(std::is_sorted(container.begin(), container.end()), (entityName, "is not sorted."));
  }
};

TransitId GetIdFromJson(json_t * obj)
{
  TransitId id;
  FromJSONObject(obj, "id", id);
  return id;
}

std::vector<TimeFromGateToStop> GetWeightsFromJson(json_t * obj)
{
  json_t * arr = base::GetJSONObligatoryField(obj, "weights");
  CHECK(json_is_array(arr), ());

  size_t const count = json_array_size(arr);
  std::vector<TimeFromGateToStop> weights;
  weights.reserve(count);

  for (size_t i = 0; i < count; ++i)
  {
    json_t * item = json_array_get(arr, i);

    TimeFromGateToStop weight;
    FromJSONObject(item, "stop_id", weight.m_stopId);
    FromJSONObject(item, "time_to_stop", weight.m_timeSeconds);
    weights.emplace_back(weight);
  }

  CHECK(!weights.empty(), ());
  return weights;
}

template <class T>
std::vector<T> GetVectorFromJson(json_t * obj, std::string const & field, bool obligatory = true)
{
  json_t * arr = base::GetJSONOptionalField(obj, field);
  if (!arr)
  {
    if (obligatory)
      CHECK(false, ("Obligatory field", field, "is absent."));

    return {};
  }

  CHECK(json_is_array(arr), ());

  size_t const count = json_array_size(arr);
  std::vector<T> elements;
  elements.reserve(count);

  for (size_t i = 0; i < count; ++i)
  {
    json_t * item = json_array_get(arr, i);
    T element = 0;
    FromJSON(item, element);
    elements.emplace_back(element);
  }

  return elements;
}

m2::PointD GetPointFromJson(json_t * obj)
{
  CHECK(json_is_object(obj), ());

  m2::PointD point;
  FromJSONObject(obj, "x", point.x);
  FromJSONObject(obj, "y", point.y);
  return point;
}

std::vector<m2::PointD> GetPointsFromJson(json_t * obj)
{
  json_t * arr = base::GetJSONObligatoryField(obj, "points");
  CHECK(json_is_array(arr), ());

  std::vector<m2::PointD> points;
  size_t const count = json_array_size(arr);
  points.reserve(count);

  for (size_t i = 0; i < count; ++i)
  {
    json_t * item = json_array_get(arr, i);
    points.emplace_back(GetPointFromJson(item));
  }

  return points;
}

TimeTable GetTimeTableFromJson(json_t * obj)
{
  json_t * arr = base::GetJSONOptionalField(obj, "timetable");
  if (!arr)
    return TimeTable{};

  CHECK(json_is_array(arr), ());

  TimeTable timetable;

  for (size_t i = 0; i < json_array_size(arr); ++i)
  {
    json_t * item = json_array_get(arr, i);
    CHECK(json_is_object(item), ());

    TransitId lineId;
    FromJSONObject(item, "line_id", lineId);

    std::vector<TimeInterval> timeIntervals;

    auto const & rawValues = GetVectorFromJson<uint64_t>(item, "intervals");
    timeIntervals.reserve(rawValues.size());
    for (auto const & rawValue : rawValues)
      timeIntervals.push_back(TimeInterval(rawValue));

    timetable[lineId] = timeIntervals;
  }

  return timetable;
}

Translations GetTranslationsFromJson(json_t * obj, std::string const & field)
{
  json_t * arr = base::GetJSONObligatoryField(obj, field);
  CHECK(json_is_array(arr), ());
  Translations translations;

  for (size_t i = 0; i < json_array_size(arr); ++i)
  {
    json_t * item = json_array_get(arr, i);
    CHECK(json_is_object(item), ());
    std::string lang;
    std::string text;
    FromJSONObject(item, "lang", lang);
    FromJSONObject(item, "text", text);
    CHECK(translations.emplace(lang, text).second, ());
  }

  return translations;
}

ShapeLink GetShapeLinkFromJson(json_t * obj)
{
  json_t * shapeLinkObj = base::GetJSONObligatoryField(obj, "shape");
  CHECK(json_is_object(shapeLinkObj), ());

  ShapeLink shapeLink;
  FromJSONObject(shapeLinkObj, "id", shapeLink.m_shapeId);
  FromJSONObject(shapeLinkObj, "start_index", shapeLink.m_startIndex);
  FromJSONObject(shapeLinkObj, "end_index", shapeLink.m_endIndex);

  return shapeLink;
}

FrequencyIntervals GetFrequencyIntervals(json_t * obj, std::string const & field)
{
  json_t * arrTimeIntervals = base::GetJSONObligatoryField(obj, field);
  size_t const countTimeIntervals = json_array_size(arrTimeIntervals);
  FrequencyIntervals frequencyIntervals;

  for (size_t i = 0; i < countTimeIntervals; ++i)
  {
    json_t * itemTimeIterval = json_array_get(arrTimeIntervals, i);

    uint64_t rawTime = 0;
    FromJSONObject(itemTimeIterval, "time_interval", rawTime);

    Frequency frequency = 0;
    FromJSONObject(itemTimeIterval, "frequency", frequency);

    frequencyIntervals.AddInterval(TimeInterval(rawTime), frequency);
  }

  return frequencyIntervals;
}

Schedule GetScheduleFromJson(json_t * obj)
{
  Schedule schedule;

  json_t * scheduleObj = base::GetJSONObligatoryField(obj, "schedule");

  Frequency defaultFrequency = 0;
  FromJSONObject(scheduleObj, "def_frequency", defaultFrequency);
  schedule.SetDefaultFrequency(defaultFrequency);

  if (json_t * arrIntervals = base::GetJSONOptionalField(scheduleObj, "intervals"); arrIntervals)
  {
    for (size_t i = 0; i < json_array_size(arrIntervals); ++i)
    {
      json_t * item = json_array_get(arrIntervals, i);
      uint32_t rawData = 0;
      FromJSONObject(item, "dates_interval", rawData);

      schedule.AddDatesInterval(DatesInterval(rawData), GetFrequencyIntervals(item, "time_intervals"));
    }
  }

  if (json_t * arrExceptions = base::GetJSONOptionalField(scheduleObj, "exceptions"); arrExceptions)
  {
    for (size_t i = 0; i < json_array_size(arrExceptions); ++i)
    {
      json_t * item = json_array_get(arrExceptions, i);
      uint32_t rawData = 0;
      FromJSONObject(item, "exception", rawData);

      schedule.AddDateException(DateException(rawData), GetFrequencyIntervals(item, "time_intervals"));
    }
  }

  return schedule;
}

std::tuple<OsmId, FeatureId, TransitId> CalculateIds(base::Json const & obj, OsmIdToFeatureIdsMap const & mapping)
{
  OsmId osmId = kInvalidOsmId;
  FeatureId featureId = kInvalidFeatureId;
  TransitId id = kInvalidTransitId;

  // Osm id is present in subway items and is absent in all other public transport items.
  FromJSONObjectOptionalField(obj.get(), "osm_id", osmId);
  FromJSONObjectOptionalField(obj.get(), "id", id);

  if (osmId == 0)
  {
    osmId = kInvalidOsmId;
  }
  else
  {
    FromJSONObjectOptionalField(obj.get(), "feature_id", featureId);
    base::GeoObjectId const geoId(osmId);
    auto const it = mapping.find(geoId);
    if (it != mapping.cend())
    {
      CHECK(!it->second.empty(),
            ("Osm id", osmId, "encoded as", geoId.GetEncodedId(), "from transit does not correspond to any feature."));
      if (it->second.size() != 1)
      {
        // |osmId| corresponds to several feature ids. It may happen in case of stops,
        // if a stop is present as a relation. It's a rare case.
        LOG(LWARNING,
            ("Osm id", osmId, "encoded as", geoId.GetEncodedId(), "corresponds to", it->second.size(), "feature ids."));
      }
      featureId = it->second[0];
    }
  }

  return {osmId, featureId, id};
}

void Read(base::Json const & obj, std::vector<Network> & networks)
{
  std::string title;
  FromJSONObject(obj.get(), "title", title);
  networks.emplace_back(GetIdFromJson(obj.get()), title);
}

void Read(base::Json const & obj, std::vector<Route> & routes)
{
  TransitId const id = GetIdFromJson(obj.get());
  TransitId networkId;
  std::string routeType;
  std::string color;
  std::string title;

  FromJSONObject(obj.get(), "network_id", networkId);
  FromJSONObject(obj.get(), "color", color);
  FromJSONObject(obj.get(), "type", routeType);
  FromJSONObject(obj.get(), "title", title);

  routes.emplace_back(id, networkId, routeType, title, color);
}

void Read(base::Json const & obj, std::vector<Line> & lines)
{
  TransitId const id = GetIdFromJson(obj.get());
  TransitId routeId;
  FromJSONObject(obj.get(), "route_id", routeId);
  ShapeLink const shapeLink = GetShapeLinkFromJson(obj.get());
  std::string title;
  FromJSONObject(obj.get(), "title", title);

  IdList const & stopIds = GetVectorFromJson<TransitId>(obj.get(), "stops_ids");
  Schedule const & schedule = GetScheduleFromJson(obj.get());

  lines.emplace_back(id, routeId, shapeLink, title, stopIds, schedule);
}

void Read(base::Json const & obj, std::vector<LineMetadata> & linesMetadata)
{
  TransitId const id = GetIdFromJson(obj.get());

  json_t * arr = base::GetJSONObligatoryField(obj.get(), "shape_segments");

  CHECK(json_is_array(arr), ());

  size_t const count = json_array_size(arr);
  LineSegmentsOrder segmentsOrder;
  segmentsOrder.reserve(count);

  for (size_t i = 0; i < count; ++i)
  {
    json_t * item = json_array_get(arr, i);

    LineSegmentOrder lineSegmentOrder;
    FromJSONObject(item, "order", lineSegmentOrder.m_order);
    FromJSONObject(item, "start_index", lineSegmentOrder.m_segment.m_startIdx);
    FromJSONObject(item, "end_index", lineSegmentOrder.m_segment.m_endIdx);
    segmentsOrder.emplace_back(lineSegmentOrder);
  }

  linesMetadata.emplace_back(id, segmentsOrder);
}

void Read(base::Json const & obj, std::vector<Stop> & stops, OsmIdToFeatureIdsMap const & mapping)
{
  auto const & [osmId, featureId, id] = CalculateIds(obj, mapping);

  std::string title;
  FromJSONObject(obj.get(), "title", title);
  TimeTable const timetable = GetTimeTableFromJson(obj.get());
  m2::PointD const point = GetPointFromJson(base::GetJSONObligatoryField(obj.get(), "point"));
  IdList const & transferIds = GetVectorFromJson<TransitId>(obj.get(), "transfer_ids", false /* obligatory */);

  stops.emplace_back(id, featureId, osmId, title, timetable, point, transferIds);
}

void Read(base::Json const & obj, std::vector<Shape> & shapes)
{
  TransitId const id = GetIdFromJson(obj.get());
  std::vector<m2::PointD> const polyline = GetPointsFromJson(obj.get());
  shapes.emplace_back(id, polyline);
}

void Read(base::Json const & obj, std::vector<Edge> & edges, EdgeIdToFeatureId & edgeFeatureIds)
{
  TransitId stopFrom = kInvalidTransitId;
  TransitId stopTo = kInvalidTransitId;

  FromJSONObject(obj.get(), "stop_id_from", stopFrom);
  FromJSONObject(obj.get(), "stop_id_to", stopTo);
  EdgeWeight weight;
  FromJSONObject(obj.get(), "weight", weight);

  TransitId lineId = 0;
  ShapeLink shapeLink;
  bool isTransfer = false;

  FromJSONObjectOptionalField(obj.get(), "line_id", lineId);

  TransitId featureId = kInvalidFeatureId;
  FromJSONObject(obj.get(), "feature_id", featureId);

  if (lineId == 0)
  {
    lineId = kInvalidTransitId;
    isTransfer = true;
  }
  else
  {
    shapeLink = GetShapeLinkFromJson(obj.get());
  }

  edges.emplace_back(stopFrom, stopTo, weight, lineId, isTransfer, shapeLink);
  edgeFeatureIds.emplace(EdgeId(stopFrom, stopTo, lineId), featureId);
}

void Read(base::Json const & obj, std::vector<Transfer> & transfers)
{
  TransitId const id = GetIdFromJson(obj.get());
  m2::PointD const & point = GetPointFromJson(base::GetJSONObligatoryField(obj.get(), "point"));
  IdList const & stopIds = GetVectorFromJson<TransitId>(obj.get(), "stops_ids");
  transfers.emplace_back(id, point, stopIds);
}

void Read(base::Json const & obj, std::vector<Gate> & gates, OsmIdToFeatureIdsMap const & mapping)
{
  auto const & [osmId, featureId, id] = CalculateIds(obj, mapping);

  std::vector<TimeFromGateToStop> const weights = GetWeightsFromJson(obj.get());

  bool isEntrance = false;
  bool isExit = false;
  FromJSONObject(obj.get(), "entrance", isEntrance);
  FromJSONObject(obj.get(), "exit", isExit);

  m2::PointD const point = GetPointFromJson(base::GetJSONObligatoryField(obj.get(), "point"));

  gates.emplace_back(id, featureId, osmId, isEntrance, isExit, weights, point);
}

template <typename... Args>
void ReadData(std::string const & path, Args &&... args)
{
  std::ifstream input;
  input.exceptions(std::ifstream::badbit);

  try
  {
    input.open(path);
    CHECK(input.is_open(), (path));
    std::string line;

    while (std::getline(input, line))
    {
      if (line.empty())
        continue;

      base::Json jsonObject(line);
      CHECK(jsonObject.get() != nullptr, ("Error parsing json from line:", line));
      Read(jsonObject, std::forward<Args>(args)...);
    }
  }
  catch (std::ifstream::failure const & se)
  {
    LOG(LERROR, ("Exception reading line-by-line json from file", path, se.what()));
  }
  catch (base::Json::Exception const & je)
  {
    LOG(LERROR, ("Exception parsing json", path, je.what()));
  }
}

void TransitData::DeserializeFromJson(std::string const & dirWithJsons, OsmIdToFeatureIdsMap const & mapping)
{
  ReadData(base::JoinPath(dirWithJsons, kNetworksFile), m_networks);
  ReadData(base::JoinPath(dirWithJsons, kRoutesFile), m_routes);
  ReadData(base::JoinPath(dirWithJsons, kLinesFile), m_lines);
  ReadData(base::JoinPath(dirWithJsons, kLinesMetadataFile), m_linesMetadata);
  ReadData(base::JoinPath(dirWithJsons, kStopsFile), m_stops, mapping);
  ReadData(base::JoinPath(dirWithJsons, kShapesFile), m_shapes);
  ReadData(base::JoinPath(dirWithJsons, kEdgesFile), m_edges, m_edgeFeatureIds);
  ReadData(base::JoinPath(dirWithJsons, kEdgesTransferFile), m_edges, m_edgeFeatureIds);
  ReadData(base::JoinPath(dirWithJsons, kTransfersFile), m_transfers);
  ReadData(base::JoinPath(dirWithJsons, kGatesFile), m_gates, mapping);
}

void TransitData::Serialize(Writer & writer)
{
  auto const startOffset = writer.Pos();

  routing::transit::Serializer<Writer> serializer(writer);
  routing::transit::FixedSizeSerializer<Writer> fixedSizeSerializer(writer);
  m_header = TransitHeader();
  m_header.m_version = static_cast<uint16_t>(TransitVersion::AllPublicTransport);
  fixedSizeSerializer(m_header);

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

  m_header.m_linesMetadataOffset = base::checked_cast<uint32_t>(writer.Pos() - startOffset);
  serializer(m_linesMetadata);

  m_header.m_shapesOffset = base::checked_cast<uint32_t>(writer.Pos() - startOffset);
  serializer(m_shapes);

  m_header.m_routesOffset = base::checked_cast<uint32_t>(writer.Pos() - startOffset);
  serializer(m_routes);

  m_header.m_networksOffset = base::checked_cast<uint32_t>(writer.Pos() - startOffset);
  serializer(m_networks);

  m_header.m_endOffset = base::checked_cast<uint32_t>(writer.Pos() - startOffset);

  // Overwriting updated header.
  CHECK(m_header.IsValid(), (m_header));
  auto const endOffset = writer.Pos();
  writer.Seek(startOffset);
  fixedSizeSerializer(m_header);
  writer.Seek(endOffset);

  LOG(LINFO, (TRANSIT_FILE_TAG, "experimental section is ready. Header:", m_header));
}

void TransitData::Deserialize(Reader & reader)
{
  DeserializeWith(reader, [this](NonOwningReaderSource & src)
  {
    ReadStops(src);
    ReadGates(src);
    ReadEdges(src);
    ReadTransfers(src);
    ReadLines(src);
    ReadLinesMetadata(src);
    ReadShapes(src);
    ReadRoutes(src);
    ReadNetworks(src);
  });
}

void TransitData::DeserializeForRouting(Reader & reader)
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

void TransitData::DeserializeForRendering(Reader & reader)
{
  DeserializeWith(reader, [this](NonOwningReaderSource & src)
  {
    ReadStops(src);
    src.Skip(m_header.m_edgesOffset - src.Pos());
    ReadEdges(src);
    src.Skip(m_header.m_transfersOffset - src.Pos());
    ReadTransfers(src);
    ReadLines(src);
    ReadLinesMetadata(src);
    ReadShapes(src);
    ReadRoutes(src);
  });
}

void TransitData::DeserializeForCrossMwm(Reader & reader)
{
  DeserializeWith(reader, [this](NonOwningReaderSource & src)
  {
    ReadStops(src);
    src.Skip(m_header.m_edgesOffset - src.Pos());
    ReadEdges(src);
  });
}

void TransitData::Clear()
{
  ClearVisitor const visitor;
  Visit(visitor);
}

void TransitData::CheckValid() const
{
  CheckValidVisitor const visitor;
  Visit(visitor);
}

void TransitData::CheckSorted() const
{
  CheckSortedVisitor const visitor;
  Visit(visitor);
}

void TransitData::CheckUnique() const
{
  CheckUniqueVisitor const visitor;
  Visit(visitor);
}

bool TransitData::IsEmpty() const
{
  // |m_transfers| and |m_gates| may be empty and it is ok.
  return m_networks.empty() || m_routes.empty() || m_lines.empty() || m_shapes.empty() || m_stops.empty() ||
         m_edges.empty();
}

void TransitData::Sort()
{
  SortVisitor const visitor;
  Visit(visitor);
}

void TransitData::SetGatePedestrianSegments(size_t gateIdx, std::vector<SingleMwmSegment> const & seg)
{
  CHECK_LESS(gateIdx, m_gates.size(), ());
  m_gates[gateIdx].SetBestPedestrianSegments(seg);
}

void TransitData::SetStopPedestrianSegments(size_t stopIdx, std::vector<SingleMwmSegment> const & seg)
{
  CHECK_LESS(stopIdx, m_stops.size(), ());
  m_stops[stopIdx].SetBestPedestrianSegments(seg);
}

void TransitData::ReadHeader(NonOwningReaderSource & src)
{
  routing::transit::FixedSizeDeserializer<NonOwningReaderSource> fixedSizeDeserializer(src);
  fixedSizeDeserializer(m_header);
  CHECK_EQUAL(src.Pos(), m_header.m_stopsOffset, ("Wrong", TRANSIT_FILE_TAG, "section format."));
  CHECK(m_header.IsValid(), ());
}

void TransitData::ReadStops(NonOwningReaderSource & src)
{
  ReadItems(m_header.m_stopsOffset, m_header.m_gatesOffset, "stops", src, m_stops);
}

void TransitData::ReadGates(NonOwningReaderSource & src)
{
  ReadItems(m_header.m_gatesOffset, m_header.m_edgesOffset, "gates", src, m_gates);
}

void TransitData::ReadEdges(NonOwningReaderSource & src)
{
  ReadItems(m_header.m_edgesOffset, m_header.m_transfersOffset, "edges", src, m_edges);
}

void TransitData::ReadTransfers(NonOwningReaderSource & src)
{
  ReadItems(m_header.m_transfersOffset, m_header.m_linesOffset, "transfers", src, m_transfers);
}

void TransitData::ReadLines(NonOwningReaderSource & src)
{
  ReadItems(m_header.m_linesOffset, m_header.m_linesMetadataOffset, "lines", src, m_lines);
}

void TransitData::ReadLinesMetadata(NonOwningReaderSource & src)
{
  ReadItems(m_header.m_linesMetadataOffset, m_header.m_shapesOffset, "linesMetadata", src, m_linesMetadata);
}

void TransitData::ReadShapes(NonOwningReaderSource & src)
{
  ReadItems(m_header.m_shapesOffset, m_header.m_routesOffset, "shapes", src, m_shapes);
}

void TransitData::ReadRoutes(NonOwningReaderSource & src)
{
  ReadItems(m_header.m_routesOffset, m_header.m_networksOffset, "routes", src, m_routes);
}

void TransitData::ReadNetworks(NonOwningReaderSource & src)
{
  ReadItems(m_header.m_networksOffset, m_header.m_endOffset, "networks", src, m_networks);
}
}  // namespace experimental
}  // namespace transit

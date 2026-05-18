#include "transit/experimental/transit_data.hpp"

#include "transit/transit_entities.hpp"
#include "transit/transit_serdes.hpp"
#include "transit/transit_version.hpp"

#include "base/assert.hpp"
#include "base/exception.hpp"
#include "base/file_name_utils.hpp"
#include "base/logging.hpp"

#include <glaze/json.hpp>

#include <algorithm>
#include <fstream>
#include <optional>
#include <string_view>
#include <tuple>

namespace transit
{
namespace experimental
{
DECLARE_EXCEPTION(JsonException, RootException);

namespace json
{
struct Point
{
  double x = 0.0;
  double y = 0.0;
};

struct ShapeLink
{
  TransitId id = kInvalidTransitId;
  uint32_t start_index = 0;
  uint32_t end_index = 0;
};

struct FrequencyInterval
{
  uint64_t time_interval = 0;
  Frequency frequency = 0;
};

struct DateInterval
{
  uint32_t dates_interval = 0;
  std::vector<FrequencyInterval> time_intervals;
};

struct ExceptionInterval
{
  uint32_t exception = 0;
  std::vector<FrequencyInterval> time_intervals;
};

struct Schedule
{
  Frequency def_frequency = 0;
  std::vector<DateInterval> intervals;
  std::vector<ExceptionInterval> exceptions;
};

struct Network
{
  TransitId id = kInvalidTransitId;
  std::string title;
};

struct Route
{
  TransitId id = kInvalidTransitId;
  TransitId network_id = kInvalidTransitId;
  std::string color;
  std::string type;
  std::string title;
};

struct Line
{
  TransitId id = kInvalidTransitId;
  TransitId route_id = kInvalidTransitId;
  ShapeLink shape;
  std::string title;
  IdList stops_ids;
  Schedule schedule;
};

struct ShapeSegment
{
  int order = 0;
  uint32_t start_index = 0;
  uint32_t end_index = 0;
};

struct LineMetadata
{
  TransitId id = kInvalidTransitId;
  std::vector<ShapeSegment> shape_segments;
};

struct TimeTableItem
{
  TransitId line_id = kInvalidTransitId;
  std::vector<uint64_t> intervals;
};

struct Stop
{
  std::optional<OsmId> osm_id;
  std::optional<FeatureId> feature_id;
  std::optional<TransitId> id;
  Point point;
  std::string title;
  std::vector<TimeTableItem> timetable;
  IdList transfer_ids;
};

struct Shape
{
  TransitId id = kInvalidTransitId;
  std::vector<Point> points;
};

struct Edge
{
  TransitId stop_id_from = kInvalidTransitId;
  TransitId stop_id_to = kInvalidTransitId;
  EdgeWeight weight = 0;
  std::optional<TransitId> line_id;
  FeatureId feature_id = kInvalidFeatureId;
  std::optional<ShapeLink> shape;
};

struct Transfer
{
  TransitId id = kInvalidTransitId;
  Point point;
  IdList stops_ids;
};

struct Gate
{
  struct Weight
  {
    TransitId stop_id = kInvalidTransitId;
    uint32_t time_to_stop = 0;
  };

  std::optional<OsmId> osm_id;
  std::optional<FeatureId> feature_id;
  std::optional<TransitId> id;
  std::vector<Weight> weights;
  bool exit = false;
  bool entrance = false;
  Point point;
};
}  // namespace json

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

template <typename T>
T ReadJson(std::string_view source)
{
  T result;
  glz::opts constexpr opts{.error_on_unknown_keys = false, .error_on_missing_keys = false};
  if (auto const error = glz::read<opts>(result, source); error)
    MYTHROW(JsonException, (glz::format_error(error, source)));
  return result;
}

m2::PointD ToPoint(json::Point const & point)
{
  return {point.x, point.y};
}

ShapeLink ToShapeLink(json::ShapeLink const & shapeLink)
{
  return {shapeLink.id, shapeLink.start_index, shapeLink.end_index};
}

FrequencyIntervals ToFrequencyIntervals(std::vector<json::FrequencyInterval> const & intervals)
{
  FrequencyIntervals frequencyIntervals;

  for (auto const & interval : intervals)
    frequencyIntervals.AddInterval(TimeInterval(interval.time_interval), interval.frequency);

  return frequencyIntervals;
}

std::vector<TimeFromGateToStop> ToWeights(std::vector<json::Gate::Weight> const & weightsJson)
{
  std::vector<TimeFromGateToStop> weights;
  weights.reserve(weightsJson.size());

  for (auto const & weightJson : weightsJson)
    weights.emplace_back(weightJson.stop_id, weightJson.time_to_stop);

  CHECK(!weights.empty(), ());
  return weights;
}

std::vector<m2::PointD> ToPoints(std::vector<json::Point> const & pointsJson)
{
  std::vector<m2::PointD> points;
  points.reserve(pointsJson.size());

  for (auto const & pointJson : pointsJson)
    points.push_back(ToPoint(pointJson));

  return points;
}

TimeTable ToTimeTable(std::vector<json::TimeTableItem> const & timetableJson)
{
  TimeTable timetable;

  for (auto const & item : timetableJson)
  {
    std::vector<TimeInterval> intervals;
    intervals.reserve(item.intervals.size());
    for (auto const & rawValue : item.intervals)
      intervals.push_back(TimeInterval(rawValue));

    timetable[item.line_id] = std::move(intervals);
  }

  return timetable;
}

Schedule ToSchedule(json::Schedule const & scheduleJson)
{
  Schedule schedule;
  schedule.SetDefaultFrequency(scheduleJson.def_frequency);

  for (auto const & interval : scheduleJson.intervals)
    schedule.AddDatesInterval(DatesInterval(interval.dates_interval), ToFrequencyIntervals(interval.time_intervals));

  for (auto const & exception : scheduleJson.exceptions)
    schedule.AddDateException(DateException(exception.exception), ToFrequencyIntervals(exception.time_intervals));

  return schedule;
}

template <typename Json>
std::tuple<OsmId, FeatureId, TransitId> CalculateIds(Json const & obj, OsmIdToFeatureIdsMap const & mapping)
{
  OsmId osmId = obj.osm_id.value_or(kInvalidOsmId);
  FeatureId featureId = obj.feature_id.value_or(kInvalidFeatureId);
  TransitId id = obj.id.value_or(kInvalidTransitId);

  // Osm id is present in subway items and is absent in all other public transport items.
  if (osmId == 0)
  {
    osmId = kInvalidOsmId;
  }
  else
  {
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

void Read(std::string_view source, std::vector<Network> & networks)
{
  auto const obj = ReadJson<json::Network>(source);
  networks.emplace_back(obj.id, obj.title);
}

void Read(std::string_view source, std::vector<Route> & routes)
{
  auto const obj = ReadJson<json::Route>(source);
  routes.emplace_back(obj.id, obj.network_id, obj.type, obj.title, obj.color);
}

void Read(std::string_view source, std::vector<Line> & lines)
{
  auto const obj = ReadJson<json::Line>(source);
  lines.emplace_back(obj.id, obj.route_id, ToShapeLink(obj.shape), obj.title, obj.stops_ids, ToSchedule(obj.schedule));
}

void Read(std::string_view source, std::vector<LineMetadata> & linesMetadata)
{
  auto const obj = ReadJson<json::LineMetadata>(source);

  LineSegmentsOrder segmentsOrder;
  segmentsOrder.reserve(obj.shape_segments.size());

  for (auto const & segment : obj.shape_segments)
  {
    LineSegmentOrder lineSegmentOrder({segment.start_index, segment.end_index}, segment.order);
    segmentsOrder.push_back(lineSegmentOrder);
  }

  linesMetadata.emplace_back(obj.id, segmentsOrder);
}

void Read(std::string_view source, std::vector<Stop> & stops, OsmIdToFeatureIdsMap const & mapping)
{
  auto const obj = ReadJson<json::Stop>(source);
  auto const & [osmId, featureId, id] = CalculateIds(obj, mapping);

  stops.emplace_back(id, featureId, osmId, obj.title, ToTimeTable(obj.timetable), ToPoint(obj.point), obj.transfer_ids);
}

void Read(std::string_view source, std::vector<Shape> & shapes)
{
  auto const obj = ReadJson<json::Shape>(source);
  shapes.emplace_back(obj.id, ToPoints(obj.points));
}

void Read(std::string_view source, std::vector<Edge> & edges, EdgeIdToFeatureId & edgeFeatureIds)
{
  auto const obj = ReadJson<json::Edge>(source);
  TransitId lineId = obj.line_id.value_or(0);
  ShapeLink shapeLink;
  bool isTransfer = false;

  if (lineId == 0)
  {
    lineId = kInvalidTransitId;
    isTransfer = true;
  }
  else
  {
    CHECK(obj.shape, ("Shape is required for non-transfer edge."));
    shapeLink = ToShapeLink(*obj.shape);
  }

  edges.emplace_back(obj.stop_id_from, obj.stop_id_to, obj.weight, lineId, isTransfer, shapeLink);
  edgeFeatureIds.emplace(EdgeId(obj.stop_id_from, obj.stop_id_to, lineId), obj.feature_id);
}

void Read(std::string_view source, std::vector<Transfer> & transfers)
{
  auto const obj = ReadJson<json::Transfer>(source);
  transfers.emplace_back(obj.id, ToPoint(obj.point), obj.stops_ids);
}

void Read(std::string_view source, std::vector<Gate> & gates, OsmIdToFeatureIdsMap const & mapping)
{
  auto const obj = ReadJson<json::Gate>(source);
  auto const & [osmId, featureId, id] = CalculateIds(obj, mapping);

  gates.emplace_back(id, featureId, osmId, obj.entrance, obj.exit, ToWeights(obj.weights), ToPoint(obj.point));
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

      Read(line, std::forward<Args>(args)...);
    }
  }
  catch (std::ifstream::failure const & se)
  {
    LOG(LERROR, ("Exception reading line-by-line json from file", path, se.what()));
  }
  catch (RootException const & je)
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

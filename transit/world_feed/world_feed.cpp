#include "transit/world_feed/world_feed.hpp"

#include "transit/transit_entities.hpp"
#include "transit/world_feed/date_time_helpers.hpp"
#include "transit/world_feed/feed_helpers.hpp"

#include "platform/platform.hpp"

#include "coding/string_utf8_multilang.hpp"

#include "base/assert.hpp"
#include "base/file_name_utils.hpp"
#include "base/logging.hpp"
#include "base/newtype.hpp"

#include <algorithm>
#include <cmath>
#include <iosfwd>
#include <memory>
#include <optional>
#include <tuple>
#include <utility>

#include "3party/boost/boost/algorithm/string.hpp"
#include "3party/boost/boost/container_hash/hash.hpp"

#include "3party/jansson/myjansson.hpp"

namespace
{
// TODO(o.khlopkova) Set average speed for each type of transit separately - trains, buses, etc.
// Average transit speed. Approximately 40 km/h.
static double constexpr kAvgTransitSpeedMpS = 11.1;
// If count of corrupted shapes in feed exceeds this value we skip feed and don't save it. The shape
// is corrupted if we cant't properly project all stops from the trip to its polyline.
static size_t constexpr kMaxInvalidShapesCount = 5;

template <class C, class ID>
void AddToRegions(C & container, ID const & id, transit::Regions const & regions)
{
  for (auto const & region : regions)
    container[region].emplace(id);
}

template <class C, class ID, class S>
transit::Regions AddToRegions(C & container, S const & splitter, ID const & id,
                              m2::PointD const & point)
{
  auto const & regions = splitter.GetAffiliations(point);
  CHECK_LESS_OR_EQUAL(
      regions.size(), 1,
      ("Point", mercator::ToLatLon(point), "belongs to multiple regions:", regions));

  AddToRegions(container, id, regions);
  return regions;
}

template <class C>
void AddToRegionsIfMatches(C & container, transit::TransitId idForAdd,
                           transit::IdsInRegion const & stopsInRegion, transit::TransitId stopId)
{
  for (auto const & [region, stopIds] : stopsInRegion)
  {
    if (stopIds.find(stopId) != stopIds.end())
      container[region].emplace(idForAdd);
  }
}

void WriteJson(json_t * node, std::ofstream & output)
{
  std::unique_ptr<char, JSONFreeDeleter> buffer(json_dumps(node, JSON_COMPACT));
  std::string record(buffer.get());
  output << record << std::endl;
}

base::JSONPtr PointToJson(m2::PointD const & point)
{
  auto coords = base::NewJSONObject();
  ToJSONObject(*coords, "x", point.x);
  ToJSONObject(*coords, "y", point.y);
  return coords;
}

base::JSONPtr ShapeLinkToJson(transit::ShapeLink const & shapeLink)
{
  auto node = base::NewJSONObject();
  ToJSONObject(*node, "id", shapeLink.m_shapeId);
  ToJSONObject(*node, "start_index", shapeLink.m_startIndex);
  ToJSONObject(*node, "end_index", shapeLink.m_endIndex);
  return node;
}

base::JSONPtr IdListToJson(transit::IdList const & stopIds)
{
  auto idArr = base::NewJSONArray();

  for (auto const & stopId : stopIds)
  {
    auto nodeId = base::NewJSONInt(stopId);
    json_array_append_new(idArr.get(), nodeId.release());
  }

  return idArr;
}

base::JSONPtr TranslationsToJson(transit::Translations const & translations)
{
  auto translationsArr = base::NewJSONArray();

  for (auto const & [lang, text] : translations)
  {
    auto translationJson = base::NewJSONObject();
    ToJSONObject(*translationJson, "lang", lang);
    ToJSONObject(*translationJson, "text", text);
    json_array_append_new(translationsArr.get(), translationJson.release());
  }

  return translationsArr;
}

template <class C, class S>
bool DumpData(C const & container, S const & idSet, std::string const & path, bool overwrite)
{
  std::ofstream output;
  output.exceptions(std::ofstream::failbit | std::ofstream::badbit);

  try
  {
    std::ios_base::openmode mode = overwrite ? std::ofstream::trunc : std::ofstream::app;
    output.open(path, mode);

    if (!output.is_open())
      return false;

    container.Write(idSet, output);
  }
  catch (std::ofstream::failure const & se)
  {
    LOG(LWARNING, ("Exception saving json to file", path, se.what()));
    return false;
  }

  return true;
}

struct StopOnShape
{
  transit::TransitId m_id = 0;
  size_t m_index = 0;
};

size_t GetStopIndex(std::unordered_map<transit::TransitId, std::vector<size_t>> const & stopIndexes,
                    transit::TransitId id, size_t index = 0)
{
  auto it = stopIndexes.find(id);
  CHECK(it != stopIndexes.end(), (id));
  CHECK(index < it->second.size(), (index, it->second.size()));

  return *(it->second.begin() + index);
}

std::pair<StopOnShape, StopOnShape> GetStopPairOnShape(
    std::unordered_map<transit::TransitId, std::vector<size_t>> const & stopIndexes,
    transit::StopsOnLines const & stopsOnLines, size_t index)
{
  auto const & stopIds = stopsOnLines.m_stopSeq;

  CHECK(!stopIds.empty(), ());
  CHECK_LESS(index, stopIds.size() - 1, ());

  StopOnShape stop1;
  StopOnShape stop2;

  stop1.m_id = stopIds[index];
  stop2.m_id = (!stopsOnLines.m_isValid && stopIds.size() == 1) ? stop1.m_id : stopIds[index + 1];

  if (stopsOnLines.m_isValid)
  {
    stop1.m_index = GetStopIndex(stopIndexes, stop1.m_id);
    stop2.m_index = stop1.m_id == stop2.m_id ? GetStopIndex(stopIndexes, stop1.m_id, 1)
                                             : GetStopIndex(stopIndexes, stop2.m_id);
  }
  return {stop1, stop2};
}

struct Link
{
  Link(transit::TransitId lineId, transit::TransitId shapeId, size_t shapeSize);
  transit::TransitId m_lineId;
  transit::TransitId m_shapeId;
  size_t m_shapeSize;
};

Link::Link(transit::TransitId lineId, transit::TransitId shapeId, size_t shapeSize)
  : m_lineId(lineId), m_shapeId(shapeId), m_shapeSize(shapeSize)
{
}
}  // namespace

namespace transit
{
// Static fields.
std::unordered_set<std::string> WorldFeed::m_agencyHashes;

EdgeId::EdgeId(TransitId fromStopId, TransitId toStopId, TransitId lineId)
  : m_fromStopId(fromStopId), m_toStopId(toStopId), m_lineId(lineId)
{
}

bool EdgeId::operator==(EdgeId const & other) const
{
  return std::tie(m_fromStopId, m_toStopId, m_lineId) ==
         std::tie(other.m_fromStopId, other.m_toStopId, other.m_lineId);
}

EdgeTransferId::EdgeTransferId(TransitId fromStopId, TransitId toStopId)
  : m_fromStopId(fromStopId), m_toStopId(toStopId)
{
}

bool EdgeTransferId::operator==(EdgeTransferId const & other) const
{
  return std::tie(m_fromStopId, m_toStopId) == std::tie(other.m_fromStopId, other.m_toStopId);
}
size_t EdgeIdHasher::operator()(EdgeId const & key) const
{
  size_t seed = 0;
  boost::hash_combine(seed, key.m_fromStopId);
  boost::hash_combine(seed, key.m_toStopId);
  boost::hash_combine(seed, key.m_lineId);
  return seed;
}

size_t EdgeTransferIdHasher::operator()(EdgeTransferId const & key) const
{
  size_t seed = 0;
  boost::hash_combine(seed, key.m_fromStopId);
  boost::hash_combine(seed, key.m_toStopId);
  return seed;
}

ShapeData::ShapeData(std::vector<m2::PointD> const & points) : m_points(points) {}

IdGenerator::IdGenerator(std::string const & idMappingPath) : m_idMappingPath(idMappingPath)
{
  LOG(LINFO, ("Inited generator with", m_curId, "start id and path to mappings", m_idMappingPath));
  CHECK(!m_idMappingPath.empty(), ());

  if (!Platform::IsFileExistsByFullPath(m_idMappingPath))
  {
    LOG(LINFO, ("Mapping", m_idMappingPath, "is not yet created."));
    return;
  }

  std::ifstream mappingFile;
  mappingFile.exceptions(std::ifstream::badbit);

  try
  {
    mappingFile.open(m_idMappingPath);
    CHECK(mappingFile.is_open(), ("Could not open", m_idMappingPath));

    std::string idStr;
    std::string hash;
    bool inserted = false;

    if (!std::getline(mappingFile, idStr))
    {
      LOG(LINFO, ("Mapping", m_idMappingPath, "is empty."));
      return;
    }

    // The first line of the mapping file is current free id.
    m_curId = static_cast<TransitId>(std::stol(idStr));

    // Next lines are sequences of id and hash pairs, each on new line.
    while (std::getline(mappingFile, idStr))
    {
      std::getline(mappingFile, hash);

      auto const id = static_cast<TransitId>(std::stol(idStr));

      std::tie(std::ignore, inserted) = m_hashToId.emplace(hash, id);
      CHECK(inserted, ("Not unique", id, hash));
    }
  }
  catch (std::ifstream::failure const & se)
  {
    LOG(LERROR, ("Exception reading file with mappings", m_idMappingPath, se.what()));
  }

  LOG(LINFO, ("Loaded", m_hashToId.size(), "hash-to-id mappings. Current free id:", m_curId));
}

TransitId IdGenerator::MakeId(const std::string & hash)
{
  CHECK(!hash.empty(), ("Empty hash cannot be added to the mapping."));

  auto [it, inserted] = m_hashToId.emplace(hash, 0);
  if (!inserted)
    return it->second;

  it->second = m_curId++;

  return it->second;
}

void IdGenerator::Save()
{
  LOG(LINFO, ("Started saving", m_hashToId.size(), "mappings to", m_idMappingPath));

  std::ofstream mappingFile;
  mappingFile.exceptions(std::ofstream::failbit | std::ofstream::badbit);

  try
  {
    mappingFile.open(m_idMappingPath, std::ofstream::out | std::ofstream::trunc);
    CHECK(mappingFile.is_open(), ("Path to the mapping file does not exist:", m_idMappingPath));

    mappingFile << m_curId << std::endl;

    for (auto const & [hash, id] : m_hashToId)
      mappingFile << id << std::endl << hash << std::endl;
  }
  catch (std::ofstream::failure const & se)
  {
    LOG(LERROR, ("Exception writing file with mappings", m_idMappingPath, se.what()));
  }
}

StopsOnLines::StopsOnLines(IdList const & ids) : m_stopSeq(ids) {}

void StopData::UpdateTimetable(TransitId lineId, gtfs::StopTime const & stopTime)
{
  bool const arrivalIsSet = stopTime.arrival_time.is_provided();
  bool const departureIsSet = stopTime.departure_time.is_provided();

  if (!arrivalIsSet && !departureIsSet)
    return;

  auto arrival = arrivalIsSet ? stopTime.arrival_time : stopTime.departure_time;
  auto departure = departureIsSet ? stopTime.departure_time : stopTime.arrival_time;

  arrival.limit_hours_to_24max();
  departure.limit_hours_to_24max();

  auto const newRuleSeq = GetRuleSequenceOsmoh(arrival, departure);
  auto [it, inserted] = m_timetable.emplace(lineId, osmoh::OpeningHours());
  if (inserted)
  {
    it->second = osmoh::OpeningHours({newRuleSeq});
    return;
  }

  auto ruleSeq = it->second.GetRule();
  ruleSeq.push_back(newRuleSeq);
  it->second = osmoh::OpeningHours({ruleSeq});
}

WorldFeed::WorldFeed(IdGenerator & generator, ColorPicker & colorPicker,
                     feature::CountriesFilesAffiliation & mwmMatcher)
  : m_idGenerator(generator), m_colorPicker(colorPicker), m_affiliation(mwmMatcher)
{
}

bool WorldFeed::FillNetworks()
{
  bool inserted = false;

  for (const auto & agency : m_feed.get_agencies())
  {
    // For one agency_name there can be multiple agency_id in the same feed.
    std::string const agencyHash = BuildHash(agency.agency_id, agency.agency_name);
    static size_t constexpr kMaxGtfsHashSize = 100;
    if (m_gtfsHash.size() + agencyHash.size() <= kMaxGtfsHashSize)
      m_gtfsHash += agencyHash;

    std::tie(std::ignore, inserted) =
        m_gtfsIdToHash[FieldIdx::AgencyIdx].emplace(agency.agency_id, agencyHash);

    if (!inserted)
    {
      LOG(LINFO, ("agency_id duplicates in same feed:", agencyHash));
      continue;
    }

    // agency_id is required when the dataset provides data for routes from more than one agency.
    // Otherwise agency_id field can be empty in routes.txt and other files. So we add it too:
    if (m_feed.get_agencies().size() == 1)
      m_gtfsIdToHash[FieldIdx::AgencyIdx].emplace("", agencyHash);

    std::tie(std::ignore, inserted) = m_agencyHashes.insert(agencyHash);
    if (!inserted)
    {
      LOG(LINFO, ("Agency hash copy from other feed:", agencyHash, "Skipped."));
      m_agencySkipList.insert(agencyHash);
    }

    Translations translation;
    translation[m_feedLanguage] = agency.agency_name;

    std::tie(std::ignore, inserted) =
        m_networks.m_data.emplace(m_idGenerator.MakeId(agencyHash), translation);
    CHECK(inserted, ());
  }

  return !m_networks.m_data.empty();
}

bool WorldFeed::FillRoutes()
{
  bool inserted = false;

  for (const auto & route : m_feed.get_routes())
  {
    // Filters irrelevant types, e.g. taxi, subway.
    if (!IsRelevantType(route.route_type))
      continue;

    std::string const routeType = ToString(route.route_type);
    // Filters unrecognized types.
    if (routeType.empty())
      continue;

    auto const itAgencyHash = m_gtfsIdToHash[FieldIdx::AgencyIdx].find(route.agency_id);
    CHECK(itAgencyHash != m_gtfsIdToHash[FieldIdx::AgencyIdx].end(), (route.agency_id));

    auto const & agencyHash = itAgencyHash->second;
    CHECK(!agencyHash.empty(), ("Empty hash for agency id:", route.agency_id));

    // Avoids duplicates of agencies with linked routes.
    if (m_agencySkipList.find(agencyHash) != m_agencySkipList.end())
      continue;

    std::string const routeHash = BuildHash(agencyHash, route.route_id);
    std::tie(std::ignore, inserted) =
        m_gtfsIdToHash[FieldIdx::RoutesIdx].emplace(route.route_id, routeHash);
    CHECK(inserted, (route.route_id, routeHash));

    RouteData data;
    data.m_networkId = m_idGenerator.MakeId(agencyHash);
    data.m_color = m_colorPicker.GetNearestColor(route.route_color);
    data.m_routeType = routeType;
    data.m_title[m_feedLanguage] =
        route.route_long_name.empty() ? route.route_short_name : route.route_long_name;

    std::tie(std::ignore, inserted) =
        m_routes.m_data.emplace(m_idGenerator.MakeId(routeHash), data);
    CHECK(inserted, ());
  }

  return !m_routes.m_data.empty();
}

bool WorldFeed::SetFeedLanguage()
{
  static std::string const kNativeForCountry = "default";

  m_feedLanguage = m_feed.get_feed_info().feed_lang;
  if (m_feedLanguage.empty())
  {
    m_feedLanguage = kNativeForCountry;
    return false;
  }

  boost::algorithm::to_lower(m_feedLanguage);

  StringUtf8Multilang multilang;
  if (multilang.GetLangIndex(m_feedLanguage) != StringUtf8Multilang::kUnsupportedLanguageCode)
    return true;

  LOG(LINFO, ("Unsupported language:", m_feedLanguage));
  m_feedLanguage = kNativeForCountry;
  return false;
}

bool WorldFeed::AddShape(GtfsIdToHash::iterator & iter, std::string const & gtfsShapeId,
                         TransitId lineId)
{
  auto const & shapeItems = m_feed.get_shape(gtfsShapeId, true);
  if (shapeItems.size() < 2)
  {
    LOG(LINFO, ("Invalid shape. Length:", shapeItems.size(), "Shape id", gtfsShapeId));
    return false;
  }

  std::string const shapeHash = BuildHash(m_gtfsHash, shapeItems[0].shape_id);
  iter->second = shapeHash;
  auto const shapeId = m_idGenerator.MakeId(shapeHash);

  auto [it, inserted] = m_shapes.m_data.emplace(shapeId, ShapeData());

  if (inserted)
  {
    std::vector<m2::PointD> points;
    // Reserve items also for future insertion of stops projections:
    points.reserve(shapeItems.size() * 1.3);

    for (auto const & point : shapeItems)
      points.push_back(mercator::FromLatLon(point.shape_pt_lat, point.shape_pt_lon));

    it->second.m_points = points;
  }

  it->second.m_lineIds.insert(lineId);

  return true;
}

bool WorldFeed::UpdateStop(TransitId stopId, gtfs::StopTime const & stopTime,
                           std::string const & stopHash, TransitId lineId)
{
  auto [it, inserted] = m_stops.m_data.emplace(stopId, StopData());

  // If the stop is already present we replenish the schedule and return.
  if (!inserted)
  {
    it->second.UpdateTimetable(lineId, stopTime);
    return true;
  }

  auto const stop = m_feed.get_stop(stopTime.stop_id);
  if (stop)
  {
    m_gtfsIdToHash[FieldIdx::StopsIdx].emplace(stopTime.stop_id, stopHash);
  }
  else
  {
    LOG(LINFO,
        ("stop is present in stop_times, but not in stops:", stopId, stopTime.stop_id, lineId));
    return false;
  }

  StopData data;
  data.m_point = mercator::FromLatLon(stop->stop_lat, stop->stop_lon);
  data.m_title[m_feedLanguage] = stop->stop_name;
  data.m_gtfsParentId = stop->parent_station;
  data.UpdateTimetable(lineId, stopTime);
  it->second = data;
  return true;
}

std::pair<TransitId, std::string> WorldFeed::GetStopIdAndHash(std::string const & stopGtfsId)
{
  std::string const stopHash = BuildHash(m_gtfsHash, stopGtfsId);
  auto const stopId = m_idGenerator.MakeId(stopHash);
  return {stopId, stopHash};
}

bool WorldFeed::FillStopsEdges()
{
  gtfs::StopTimes allStopTimes = m_feed.get_stop_times();
  std::sort(
      allStopTimes.begin(), allStopTimes.end(),
      [](const gtfs::StopTime & t1, const gtfs::StopTime & t2) { return t1.trip_id < t2.trip_id; });

  std::vector<std::unordered_map<TransitId, LineData>::iterator> linesForRemoval;

  for (auto it = m_lines.m_data.begin(); it != m_lines.m_data.end(); ++it)
  {
    auto const & lineId = it->first;
    LineData & lineData = it->second;
    TransitId const & shapeId = lineData.m_shapeLink.m_shapeId;
    gtfs::StopTimes stopTimes = GetStopTimesForTrip(allStopTimes, lineData.m_gtfsTripId);

    if (stopTimes.size() < 2)
    {
      LOG(LINFO, ("Invalid stop times count for trip:", stopTimes.size(), lineData.m_gtfsTripId));
      linesForRemoval.push_back(it);
      continue;
    }

    lineData.m_stopIds.reserve(stopTimes.size());

    for (size_t i = 0; i < stopTimes.size() - 1; ++i)
    {
      auto const & stopTime1 = stopTimes[i];
      auto const & stopTime2 = stopTimes[i + 1];

      // This situation occurs even in valid GTFS feeds.
      if (stopTime1.stop_id == stopTime2.stop_id && stopTimes.size() > 2)
        continue;

      auto [stop1Id, stop1Hash] = GetStopIdAndHash(stopTime1.stop_id);
      auto [stop2Id, stop2Hash] = GetStopIdAndHash(stopTime2.stop_id);

      lineData.m_stopIds.push_back(stop1Id);
      if (!UpdateStop(stop1Id, stopTime1, stop1Hash, lineId))
        return false;

      if (i == stopTimes.size() - 2)
      {
        lineData.m_stopIds.push_back(stop2Id);
        if (!UpdateStop(stop2Id, stopTime2, stop2Hash, lineId))
          return false;
      }

      EdgeData data;
      data.m_shapeLink.m_shapeId = shapeId;
      data.m_weight =
          static_cast<transit::EdgeWeight>(stopTime2.arrival_time.get_total_seconds() -
                                           stopTime1.departure_time.get_total_seconds());

      if (data.m_weight == 0)
      {
        double const distBetweenStopsM =
            stopTime2.shape_dist_traveled - stopTime1.shape_dist_traveled;
        if (distBetweenStopsM > 0)
          data.m_weight = std::ceil(distBetweenStopsM / kAvgTransitSpeedMpS);
      }

      auto [itEdge, insertedEdge] = m_edges.m_data.emplace(EdgeId(stop1Id, stop2Id, lineId), data);

      // There can be two identical pairs of stops on the same trip.
      if (!insertedEdge)
      {
        CHECK_EQUAL(itEdge->second.m_shapeLink.m_shapeId, data.m_shapeLink.m_shapeId,
                    (stopTime1.stop_id, stopTime2.stop_id));

        // We choose the most pessimistic alternative.
        if (data.m_weight > itEdge->second.m_weight)
          itEdge->second = data;
      }
    }
  }

  for (auto & it : linesForRemoval)
    m_lines.m_data.erase(it);

  return !m_edges.m_data.empty();
}

bool WorldFeed::FillLinesAndShapes()
{
  for (const auto & trip : m_feed.get_trips())
  {
    // We skip routes filtered on the route preparation stage.
    auto const itRoute = m_gtfsIdToHash[FieldIdx::RoutesIdx].find(trip.route_id);
    if (itRoute == m_gtfsIdToHash[FieldIdx::RoutesIdx].end())
      continue;

    // Skip trips with corrupted shapes.
    if (trip.shape_id.empty())
      continue;

    std::string const & routeHash = itRoute->second;
    std::string const lineHash = BuildHash(routeHash, trip.trip_id);
    auto const lineId = m_idGenerator.MakeId(lineHash);

    auto [itShape, insertedShape] = m_gtfsIdToHash[ShapesIdx].emplace(trip.shape_id, "");

    // Skip invalid shape.
    if (!insertedShape && itShape->second.empty())
      continue;

    if (insertedShape)
    {
      // Skip trips with corrupted shapes.
      if (!AddShape(itShape, trip.shape_id, lineId))
        continue;
    }

    auto [it, inserted] = m_lines.m_data.emplace(lineId, LineData());
    if (!inserted)
    {
      LOG(LINFO, ("Duplicate trip_id:", trip.trip_id, m_gtfsHash));
      return false;
    }

    TransitId const shapeId = m_idGenerator.MakeId(itShape->second);

    LineData data;
    data.m_title[m_feedLanguage] = trip.trip_short_name;
    data.m_routeId = m_idGenerator.MakeId(routeHash);
    data.m_shapeId = shapeId;
    data.m_gtfsTripId = trip.trip_id;
    data.m_gtfsServiceId = trip.service_id;
    // |m_stopIds|, |m_intervals| and |m_serviceDays| will be filled on the next steps.
    it->second = data;

    m_gtfsIdToHash[TripsIdx].emplace(trip.trip_id, lineHash);
  }

  return !m_lines.m_data.empty() && !m_shapes.m_data.empty();
}

void WorldFeed::ModifyLinesAndShapes()
{
  std::vector<Link> links;
  links.reserve(m_lines.m_data.size());

  for (auto const & [lineId, lineData] : m_lines.m_data)
  {
    links.emplace_back(lineId, lineData.m_shapeId,
                       m_shapes.m_data[lineData.m_shapeId].m_points.size());
  }

  // We sort links by shape length so we could search for shapes in which i-th shape is included
  // only in the left part of the array [0, i).
  std::sort(links.begin(), links.end(), [](Link const & link1, Link const & link2) {
    return link1.m_shapeSize > link2.m_shapeSize;
  });

  size_t subShapesCount = 0;

  // Shape ids of shapes fully contained in other shapes.
  IdSet shapesForRemoval;

  // Shape id matching to the line id linked to this shape id.
  std::unordered_map<TransitId, TransitId> matchingCache;

  for (size_t i = 1; i < links.size(); ++i)
  {
    auto const lineId = links[i].m_lineId;
    auto & lineData = m_lines.m_data[lineId];
    auto const shapeId = links[i].m_shapeId;

    auto [itCache, inserted] = matchingCache.emplace(shapeId, 0);
    if (!inserted)
    {
      if (itCache->second != 0)
      {
        lineData.m_shapeId = 0;
        lineData.m_shapeLink = m_lines.m_data[itCache->second].m_shapeLink;
      }
      continue;
    }

    auto const & points = m_shapes.m_data[shapeId].m_points;

    for (size_t j = 0; j < i; ++j)
    {
      auto const & curLineId = links[j].m_lineId;

      // We skip shapes which are already included to other shapes.
      if (m_lines.m_data[curLineId].m_shapeId == 0)
        continue;

      auto const curShapeId = links[j].m_shapeId;

      if (curShapeId == shapeId)
        continue;

      auto const & curPoints = m_shapes.m_data[curShapeId].m_points;

      auto const it = std::search(curPoints.begin(), curPoints.end(), points.begin(), points.end());

      if (it == curPoints.end())
        continue;

      // Shape with |points| polyline is fully contained in the shape with |curPoints| polyline.
      lineData.m_shapeId = 0;
      lineData.m_shapeLink.m_shapeId = curShapeId;
      lineData.m_shapeLink.m_startIndex = std::distance(curPoints.begin(), it);
      lineData.m_shapeLink.m_endIndex = lineData.m_shapeLink.m_startIndex + points.size() - 1;
      itCache->second = lineId;
      shapesForRemoval.insert(shapeId);
      ++subShapesCount;
      break;
    }
  }

  for (auto & [lineId, lineData] : m_lines.m_data)
  {
    if (lineData.m_shapeId == 0)
      continue;

    lineData.m_shapeLink.m_shapeId = lineData.m_shapeId;
    lineData.m_shapeLink.m_startIndex = 0;
    lineData.m_shapeLink.m_endIndex = m_shapes.m_data[lineData.m_shapeId].m_points.size();

    lineData.m_shapeId = 0;
  }

  for (auto const shapeId : shapesForRemoval)
    m_shapes.m_data.erase(shapeId);

  LOG(LINFO, ("Deleted", subShapesCount, "sub-shapes.", m_shapes.m_data.size(), "left."));
}

void WorldFeed::GetCalendarDates(osmoh::TRuleSequences & rules, CalendarCache & cache,
                                 std::string const & serviceId)
{
  auto [it, inserted] = cache.emplace(serviceId, osmoh::TRuleSequences());
  if (inserted)
  {
    if (auto serviceDays = m_feed.get_calendar(serviceId); serviceDays)
    {
      GetServiceDaysOsmoh(serviceDays.value(), it->second);
      MergeRules(rules, it->second);
    }
  }
  else
  {
    MergeRules(rules, it->second);
  }
}

void WorldFeed::GetCalendarDatesExceptions(osmoh::TRuleSequences & rules, CalendarCache & cache,
                                           std::string const & serviceId)
{
  auto [it, inserted] = cache.emplace(serviceId, osmoh::TRuleSequences());
  if (inserted)
  {
    auto exceptionDates = m_feed.get_calendar_dates(serviceId);
    GetServiceDaysExceptionsOsmoh(exceptionDates, it->second);
  }

  MergeRules(rules, it->second);
}

LineIntervals WorldFeed::GetFrequencies(std::unordered_map<std::string, LineIntervals> & cache,
                                        std::string const & tripId)
{
  auto [it, inserted] = cache.emplace(tripId, LineIntervals{});
  if (!inserted)
  {
    return it->second;
  }

  auto const & frequencies = m_feed.get_frequencies(tripId);
  if (frequencies.empty())
    return it->second;

  std::unordered_map<size_t, osmoh::TRuleSequences> intervals;
  for (auto const & freq : frequencies)
  {
    osmoh::RuleSequence const seq = GetRuleSequenceOsmoh(freq.start_time, freq.end_time);
    intervals[freq.headway_secs].push_back(seq);
  }

  for (auto const & [headwayS, rules] : intervals)
  {
    LineInterval interval;
    interval.m_headwayS = headwayS;
    interval.m_timeIntervals = osmoh::OpeningHours(rules);
    it->second.push_back(interval);
  }
  return it->second;
}

bool WorldFeed::FillLinesSchedule()
{
  // Service id - to - rules mapping based on GTFS calendar.
  CalendarCache cachedCalendar;
  // Service id - to - rules mapping based on GTFS calendar dates.
  CalendarCache cachedCalendarDates;

  // Trip id - to - headways for trips.
  std::unordered_map<std::string, LineIntervals> cachedFrequencies;

  for (auto & [lineId, lineData] : m_lines.m_data)
  {
    osmoh::TRuleSequences rulesDates;
    auto const & serviceId = lineData.m_gtfsServiceId;

    GetCalendarDates(rulesDates, cachedCalendar, serviceId);
    GetCalendarDatesExceptions(rulesDates, cachedCalendarDates, serviceId);

    lineData.m_serviceDays = osmoh::OpeningHours(rulesDates);

    auto const & tripId = lineData.m_gtfsTripId;
    lineData.m_intervals = GetFrequencies(cachedFrequencies, tripId);
  }

  return !cachedCalendar.empty() || !cachedCalendarDates.empty();
}

bool WorldFeed::ProjectStopsToShape(
    ShapesIter & itShape, StopsOnLines const & stopsOnLines,
    std::unordered_map<TransitId, std::vector<size_t>> & stopsToIndexes)
{
  IdList const & stopIds = stopsOnLines.m_stopSeq;
  TransitId const shapeId = itShape->first;
  auto & shape = itShape->second.m_points;
  std::optional<m2::PointD> prevPoint = std::nullopt;

  for (size_t i = 0; i < stopIds.size(); ++i)
  {
    auto const & stopId = stopIds[i];
    auto const itStop = m_stops.m_data.find(stopId);
    CHECK(itStop != m_stops.m_data.end(), (stopId));
    auto const & stop = itStop->second;

    size_t const startIdx = i == 0 ? 0 : stopsToIndexes[stopIds[i - 1]].back();
    auto const [curIdx, pointInserted] =
        PrepareNearestPointOnTrack(stop.m_point, prevPoint, startIdx, shape);

    if (curIdx > shape.size())
    {
      CHECK(!itShape->second.m_lineIds.empty(), (shapeId));
      TransitId const lineId = *stopsOnLines.m_lines.begin();

      LOG(LWARNING,
          ("Error projecting stops to the shape. GTFS trip id", m_lines.m_data[lineId].m_gtfsTripId,
           "shapeId", shapeId, "stopId", stopId, "i", i, "start index on shape", startIdx,
           "trips count", stopsOnLines.m_lines.size()));
      return false;
    }

    prevPoint = std::optional<m2::PointD>(stop.m_point);

    if (pointInserted)
    {
      for (auto & indexesList : stopsToIndexes)
      {
        for (auto & stopIndex : indexesList.second)
        {
          if (stopIndex >= curIdx)
            ++stopIndex;
        }
      }

      for (auto const & lineId : m_shapes.m_data[shapeId].m_lineIds)
      {
        auto & line = m_lines.m_data[lineId];

        if (line.m_shapeLink.m_startIndex >= curIdx)
          ++line.m_shapeLink.m_startIndex;

        if (line.m_shapeLink.m_endIndex >= curIdx)
          ++line.m_shapeLink.m_endIndex;
      }
    }

    stopsToIndexes[stopId].push_back(curIdx);
  }

  return true;
}

std::unordered_map<TransitId, std::vector<StopsOnLines>> WorldFeed::GetStopsForShapeMatching()
{
  // Shape id and list of stop sequences matched to the corresponding lines.
  std::unordered_map<TransitId, std::vector<StopsOnLines>> stopsOnShapes;

  // We build lists of stops relevant to corresponding shapes. There could be multiple different
  // stops lists linked to the same shape.
  // Example: line 8, shapeId 52, stops: [25, 26, 27]. line 9, shapeId 52, stops: [10, 9, 8, 7, 6].

  for (auto const & [lineId, lineData] : m_lines.m_data)
  {
    auto & shapeData = stopsOnShapes[lineData.m_shapeLink.m_shapeId];
    bool found = false;

    for (auto & stopsOnLines : shapeData)
    {
      if (stopsOnLines.m_stopSeq == lineData.m_stopIds)
      {
        found = true;
        stopsOnLines.m_lines.insert(lineId);
        break;
      }
    }

    if (!found)
    {
      StopsOnLines stopsOnLines(lineData.m_stopIds);
      stopsOnLines.m_lines.insert(lineId);
      shapeData.emplace_back(stopsOnLines);
    }
  }

  return stopsOnShapes;
}

size_t WorldFeed::ModifyShapes()
{
  auto stopsOnShapes = GetStopsForShapeMatching();
  size_t invalidStopSequences = 0;

  for (auto & [shapeId, stopsLists] : stopsOnShapes)
  {
    CHECK(!stopsLists.empty(), (shapeId));

    auto itShape = m_shapes.m_data.find(shapeId);
    CHECK(itShape != m_shapes.m_data.end(), (shapeId));

    std::unordered_map<TransitId, std::vector<size_t>> stopToShapeIndex;

    for (auto & stopsOnLines : stopsLists)
    {
      if (stopsOnLines.m_stopSeq.size() < 2)
      {
        TransitId const lineId = *stopsOnLines.m_lines.begin();
        LOG(LWARNING, ("Error in stops count. Lines count:", stopsOnLines.m_stopSeq.size(),
                       "GTFS trip id:", m_lines.m_data[lineId].m_gtfsTripId));
        stopsOnLines.m_isValid = false;
        ++invalidStopSequences;
      }
      else if (!ProjectStopsToShape(itShape, stopsOnLines, stopToShapeIndex))
      {
        stopsOnLines.m_isValid = false;
        ++invalidStopSequences;
      }

      if (invalidStopSequences > kMaxInvalidShapesCount)
        return invalidStopSequences;
    }

    for (auto const & stopsOnLines : stopsLists)
    {
      IdList const & stopIds = stopsOnLines.m_stopSeq;
      auto const & lineIds = stopsOnLines.m_lines;
      auto indexes = stopToShapeIndex;

      for (size_t i = 0; i < stopIds.size() - 1; ++i)
      {
        auto const [stop1, stop2] = GetStopPairOnShape(indexes, stopsOnLines, i);

        for (auto const lineId : lineIds)
        {
          if (!stopsOnLines.m_isValid)
            m_lines.m_data.erase(lineId);

          // Update |EdgeShapeLink| with shape segment start and end points.
          auto itEdge = m_edges.m_data.find(EdgeId(stop1.m_id, stop2.m_id, lineId));
          if (itEdge == m_edges.m_data.end())
            continue;

          if (stopsOnLines.m_isValid)
          {
            itEdge->second.m_shapeLink.m_startIndex = stop1.m_index;
            itEdge->second.m_shapeLink.m_endIndex = stop2.m_index;
          }
          else
          {
            m_edges.m_data.erase(itEdge);
          }
        }

        if (indexes[stop1.m_id].size() > 1)
          indexes[stop1.m_id].erase(indexes[stop1.m_id].begin());
      }
    }
  }

  return invalidStopSequences;
}

void WorldFeed::FillTransfers()
{
  bool inserted = false;

  for (auto const & transfer : m_feed.get_transfers())
  {
    if (transfer.transfer_type == gtfs::TransferType::NotPossible)
      continue;

    // Check that the two stops from the transfer are present in the global feed.
    auto const itStop1 = m_gtfsIdToHash[FieldIdx::StopsIdx].find(transfer.from_stop_id);
    if (itStop1 == m_gtfsIdToHash[FieldIdx::StopsIdx].end())
      continue;

    auto const itStop2 = m_gtfsIdToHash[FieldIdx::StopsIdx].find(transfer.to_stop_id);
    if (itStop2 == m_gtfsIdToHash[FieldIdx::StopsIdx].end())
      continue;

    TransitId const & stop1Id = m_idGenerator.MakeId(itStop1->second);
    TransitId const & stop2Id = m_idGenerator.MakeId(itStop2->second);

    // Usual case in GTFS feeds (don't ask why). We skip duplicates.
    if (stop1Id == stop2Id)
      continue;

    std::string const transitHash = BuildHash(itStop1->second, itStop2->second);
    TransitId const transitId = m_idGenerator.MakeId(transitHash);

    TransferData data;
    data.m_stopsIds = {stop1Id, stop2Id};

    auto & stop1 = m_stops.m_data.at(stop1Id);
    auto & stop2 = m_stops.m_data.at(stop2Id);

    // Coordinate of the transfer is the midpoint between two stops.
    data.m_point = stop1.m_point.Mid(stop2.m_point);

    if (m_transfers.m_data.emplace(transitId, data).second)
    {
      EdgeTransferId const transferId(stop1Id, stop2Id);

      std::tie(std::ignore, inserted) =
          m_edgesTransfers.m_data.emplace(transferId, transfer.min_transfer_time);

      LinkTransferIdToStop(stop1, transitId);
      LinkTransferIdToStop(stop2, transitId);

      if (!inserted)
        LOG(LWARNING, ("Transfers copy", transfer.from_stop_id, transfer.to_stop_id));
    }
  }
}

void WorldFeed::FillGates()
{
  std::unordered_map<std::string, std::vector<GateData>> parentToGates;
  for (auto const & stop : m_feed.get_stops())
  {
    if (stop.location_type == gtfs::StopLocationType::EntranceExit && !stop.parent_station.empty())
    {
      GateData gate;
      gate.m_gtfsId = stop.stop_id;
      gate.m_point = mercator::FromLatLon(stop.stop_lat, stop.stop_lon);
      gate.m_isEntrance = true;
      gate.m_isExit = true;
      parentToGates[stop.parent_station].emplace_back(gate);
    }
  }

  if (parentToGates.empty())
    return;

  for (auto & [stopId, stopData] : m_stops.m_data)
  {
    if (stopData.m_gtfsParentId.empty())
      continue;

    auto it = parentToGates.find(stopData.m_gtfsParentId);
    if (it == parentToGates.end())
      continue;

    for (auto & gate : it->second)
    {
      TimeFromGateToStop weight;
      weight.m_stopId = stopId;
      // We do not divide distance by average speed because average speed in stations, terminals,
      // etc is roughly 1 meter/second.
      weight.m_timeSeconds = mercator::DistanceOnEarth(stopData.m_point, gate.m_point);

      gate.m_weights.emplace_back(weight);
    }
  }

  for (auto const & pairParentGates : parentToGates)
  {
    auto const & gates = pairParentGates.second;
    for (auto const & gate : gates)
    {
      TransitId id;
      std::tie(id, std::ignore) = GetStopIdAndHash(gate.m_gtfsId);
      m_gates.m_data.emplace(id, gate);
    }
  }
}

bool WorldFeed::SpeedExceedsMaxVal(EdgeId const & edgeId, EdgeData const & edgeData)
{
  m2::PointD const & stop1 = m_stops.m_data.at(edgeId.m_fromStopId).m_point;
  m2::PointD const & stop2 = m_stops.m_data.at(edgeId.m_toStopId).m_point;

  static double const maxSpeedMpS = KmphToMps(routing::kTransitMaxSpeedKMpH);
  double const speedMpS = mercator::DistanceOnEarth(stop1, stop2) / edgeData.m_weight;

  bool speedExceedsMaxVal = speedMpS > maxSpeedMpS;
  if (speedExceedsMaxVal)
  {
    LOG(LWARNING,
        ("Invalid edge weight conflicting with kTransitMaxSpeedKMpH:", edgeId.m_fromStopId,
         edgeId.m_toStopId, edgeId.m_lineId, "speed (km/h):", MpsToKmph(speedMpS),
         "maxSpeed (km/h):", routing::kTransitMaxSpeedKMpH));
  }

  return speedExceedsMaxVal;
}

bool WorldFeed::ClearFeedByLineIds(std::unordered_set<TransitId> const & corruptedLineIds)
{
  std::unordered_set<TransitId> corruptedRouteIds;
  std::unordered_set<TransitId> corruptedShapeIds;
  std::unordered_set<TransitId> corruptedNetworkIds;

  for (auto lineId : corruptedLineIds)
  {
    LineData const & lineData = m_lines.m_data[lineId];
    corruptedRouteIds.emplace(lineData.m_routeId);
    corruptedNetworkIds.emplace(m_routes.m_data.at(lineData.m_routeId).m_networkId);
    corruptedShapeIds.emplace(lineData.m_shapeId);
  }

  for (auto const & [lineId, lineData] : m_lines.m_data)
  {
    if (corruptedLineIds.find(lineId) != corruptedLineIds.end())
      continue;

    // We keep in lists for deletion only ids which are not linked to valid entities.
    DeleteIfExists(corruptedRouteIds, lineData.m_routeId);
    DeleteIfExists(corruptedShapeIds, lineData.m_shapeId);
  }

  for (auto const & [routeId, routeData] : m_routes.m_data)
  {
    if (corruptedRouteIds.find(routeId) == corruptedRouteIds.end())
      DeleteIfExists(corruptedNetworkIds, routeData.m_networkId);
  }

  DeleteAllEntriesByIds(m_shapes.m_data, corruptedShapeIds);
  DeleteAllEntriesByIds(m_routes.m_data, corruptedRouteIds);
  DeleteAllEntriesByIds(m_networks.m_data, corruptedNetworkIds);
  DeleteAllEntriesByIds(m_lines.m_data, corruptedLineIds);

  std::unordered_set<TransitId> corruptedStopIds;

  // We fill |corruptedStopIds| and delete corresponding edges from |m_edges|.
  for (auto it = m_edges.m_data.begin(); it != m_edges.m_data.end();)
  {
    if (corruptedLineIds.find(it->first.m_lineId) != corruptedLineIds.end())
    {
      corruptedStopIds.emplace(it->first.m_fromStopId);
      corruptedStopIds.emplace(it->first.m_toStopId);
      it = m_edges.m_data.erase(it);
    }
    else
    {
      ++it;
    }
  }

  // We remove transfer edges linked to the corrupted stop ids.
  for (auto it = m_edgesTransfers.m_data.begin(); it != m_edgesTransfers.m_data.end();)
  {
    if (corruptedStopIds.find(it->first.m_fromStopId) != corruptedStopIds.end() ||
        corruptedStopIds.find(it->first.m_toStopId) != corruptedStopIds.end())
    {
      it = m_edgesTransfers.m_data.erase(it);
    }
    else
    {
      ++it;
    }
  }

  // We remove transfers linked to the corrupted stop ids.
  for (auto it = m_transfers.m_data.begin(); it != m_transfers.m_data.end();)
  {
    auto & transferData = it->second;
    DeleteAllEntriesByIds(transferData.m_stopsIds, corruptedStopIds);

    if (transferData.m_stopsIds.size() < 2)
      it = m_transfers.m_data.erase(it);
    else
      ++it;
  }

  // We remove gates linked to the corrupted stop ids.
  for (auto it = m_gates.m_data.begin(); it != m_gates.m_data.end();)
  {
    auto & gateData = it->second;

    for (auto itW = gateData.m_weights.begin(); itW != gateData.m_weights.end();)
    {
      if (corruptedStopIds.find(itW->m_stopId) != corruptedStopIds.end())
        itW = gateData.m_weights.erase(itW);
      else
        ++itW;
    }

    if (gateData.m_weights.empty())
      it = m_gates.m_data.erase(it);
    else
      ++it;
  }

  DeleteAllEntriesByIds(m_stops.m_data, corruptedStopIds);

  LOG(LINFO, ("Count of lines linked to the corrupted edges:", corruptedLineIds.size(),
              ", routes:", corruptedRouteIds.size(), ", networks:", corruptedNetworkIds.size(),
              ", shapes:", corruptedShapeIds.size()));

  return !m_networks.m_data.empty() && !m_routes.m_data.empty() && !m_lines.m_data.empty() &&
         !m_stops.m_data.empty() && !m_edges.m_data.empty();
}

bool WorldFeed::UpdateEdgeWeights()
{
  std::unordered_set<TransitId> corruptedLineIds;

  for (auto & [edgeId, edgeData] : m_edges.m_data)
  {
    if (edgeData.m_weight == 0)
    {
      auto const & polyLine = m_shapes.m_data.at(edgeData.m_shapeLink.m_shapeId).m_points;

      bool const isInverted = edgeData.m_shapeLink.m_startIndex > edgeData.m_shapeLink.m_endIndex;

      size_t const startIndex =
          isInverted ? edgeData.m_shapeLink.m_endIndex : edgeData.m_shapeLink.m_startIndex;
      size_t const endIndex =
          isInverted ? edgeData.m_shapeLink.m_startIndex : edgeData.m_shapeLink.m_endIndex;

      auto const edgePolyLine =
          std::vector<m2::PointD>(polyLine.begin() + startIndex, polyLine.begin() + endIndex + 1);

      if (edgePolyLine.size() < 1)
      {
        LOG(LWARNING, ("Invalid edge with too short shape polyline:", edgeId.m_fromStopId,
                       edgeId.m_toStopId, edgeId.m_lineId));
        corruptedLineIds.emplace(edgeId.m_lineId);
        continue;
      }

      double edgeLengthM = 0.0;

      for (size_t i = 0; i < edgePolyLine.size() - 1; ++i)
        edgeLengthM += mercator::DistanceOnEarth(edgePolyLine[i], edgePolyLine[i + 1]);

      if (edgeLengthM == 0.0)
      {
        LOG(LWARNING, ("Invalid edge with 0 length:", edgeId.m_fromStopId, edgeId.m_toStopId,
                       edgeId.m_lineId));
        corruptedLineIds.emplace(edgeId.m_lineId);
        continue;
      }

      edgeData.m_weight = std::ceil(edgeLengthM / kAvgTransitSpeedMpS);
    }

    // We check that edge weight doesn't violate A* invariant in routing runtime.
    if (SpeedExceedsMaxVal(edgeId, edgeData))
      corruptedLineIds.emplace(edgeId.m_lineId);
  }

  if (!corruptedLineIds.empty())
    return ClearFeedByLineIds(corruptedLineIds);

  return true;
}

bool WorldFeed::SetFeed(gtfs::Feed && feed)
{
  m_feed = std::move(feed);
  m_gtfsIdToHash.resize(FieldIdx::IdxCount);

  // The order of the calls is important. First we set default feed language. Then fill networks.
  // Then, based on network ids, we generate routes and so on.

  SetFeedLanguage();

  if (!FillNetworks())
  {
    LOG(LWARNING, ("Could not fill networks."));
    return false;
  }
  LOG(LINFO, ("Filled networks."));

  if (!FillRoutes())
  {
    LOG(LWARNING, ("Could not fill routes."));
    return false;
  }
  LOG(LINFO, ("Filled routes."));

  if (!FillLinesAndShapes())
  {
    LOG(LWARNING, ("Could not fill lines.", m_lines.m_data.size()));
    return false;
  }
  LOG(LINFO, ("Filled lines and shapes."));

  ModifyLinesAndShapes();
  LOG(LINFO, ("Modified lines and shapes."));

  if (!FillLinesSchedule())
  {
    LOG(LWARNING, ("Could not fill schedule for lines."));
    return false;
  }
  LOG(LINFO, ("Filled schedule for lines."));

  if (!FillStopsEdges())
  {
    LOG(LWARNING, ("Could not fill stops", m_stops.m_data.size()));
    return false;
  }
  LOG(LINFO, ("Filled stop timetables and road graph edges."));

  size_t const badShapesCount = ModifyShapes();
  LOG(LINFO, ("Modified shapes."));

  if (badShapesCount > kMaxInvalidShapesCount)
  {
    LOG(LINFO, ("Corrupted shapes count exceeds allowable limit."));
    return false;
  }

  FillTransfers();
  LOG(LINFO, ("Filled transfers."));

  FillGates();
  LOG(LINFO, ("Filled gates."));

  if (!UpdateEdgeWeights())
  {
    LOG(LWARNING, ("Found inconsistencies while updating edge weights."));
    return false;
  }

  LOG(LINFO, ("Updated edges weights."));
  return true;
}

void Networks::Write(IdSet const & ids, std::ofstream & stream) const
{
  for (auto networkId : ids)
  {
    auto const & networkTitle = m_data.find(networkId)->second;

    auto node = base::NewJSONObject();
    ToJSONObject(*node, "id", networkId);
    json_object_set_new(node.get(), "title", TranslationsToJson(networkTitle).release());

    WriteJson(node.get(), stream);
  }
}

void Routes::Write(IdSet const & ids, std::ofstream & stream) const
{
  for (auto routeId : ids)
  {
    auto const & route = m_data.find(routeId)->second;
    auto node = base::NewJSONObject();
    ToJSONObject(*node, "id", routeId);
    ToJSONObject(*node, "network_id", route.m_networkId);
    ToJSONObject(*node, "color", route.m_color);
    ToJSONObject(*node, "type", route.m_routeType);
    json_object_set_new(node.get(), "title", TranslationsToJson(route.m_title).release());

    WriteJson(node.get(), stream);
  }
}

void Lines::Write(std::unordered_map<TransitId, IdList> const & ids, std::ofstream & stream) const
{
  for (auto const & [lineId, stopIds] : ids)
  {
    auto const & line = m_data.find(lineId)->second;
    auto node = base::NewJSONObject();
    ToJSONObject(*node, "id", lineId);
    ToJSONObject(*node, "route_id", line.m_routeId);
    json_object_set_new(node.get(), "shape", ShapeLinkToJson(line.m_shapeLink).release());

    json_object_set_new(node.get(), "title", TranslationsToJson(line.m_title).release());
    // Save only stop ids inside current region.
    json_object_set_new(node.get(), "stops_ids", IdListToJson(stopIds).release());
    ToJSONObject(*node, "service_days", ToString(line.m_serviceDays));

    auto intervalsArr = base::NewJSONArray();

    for (auto const & [intervalS, openingHours] : line.m_intervals)
    {
      auto scheduleItem = base::NewJSONObject();
      ToJSONObject(*scheduleItem, "interval_s", intervalS);
      ToJSONObject(*scheduleItem, "service_hours", ToString(openingHours));
      json_array_append_new(intervalsArr.get(), scheduleItem.release());
    }

    json_object_set_new(node.get(), "intervals", intervalsArr.release());

    WriteJson(node.get(), stream);
  }
}

void Shapes::Write(IdSet const & ids, std::ofstream & stream) const
{
  for (auto shapeId : ids)
  {
    auto const & shape = m_data.find(shapeId)->second;
    auto node = base::NewJSONObject();
    ToJSONObject(*node, "id", shapeId);
    auto pointsArr = base::NewJSONArray();

    for (auto const & point : shape.m_points)
      json_array_append_new(pointsArr.get(), PointToJson(point).release());

    json_object_set_new(node.get(), "points", pointsArr.release());

    WriteJson(node.get(), stream);
  }
}

void Stops::Write(IdSet const & ids, std::ofstream & stream) const
{
  for (auto stopId : ids)
  {
    auto const & stop = m_data.find(stopId)->second;
    auto node = base::NewJSONObject();
    if (stop.m_osmId == 0)
      ToJSONObject(*node, "id", stopId);
    else
      ToJSONObject(*node, "osm_id", stop.m_osmId);

    json_object_set_new(node.get(), "point", PointToJson(stop.m_point).release());
    json_object_set_new(node.get(), "title", TranslationsToJson(stop.m_title).release());

    auto timeTableArr = base::NewJSONArray();

    for (auto const & [lineId, schedule] : stop.m_timetable)
    {
      auto scheduleItem = base::NewJSONObject();
      ToJSONObject(*scheduleItem, "line_id", lineId);
      ToJSONObject(*scheduleItem, "arrivals", ToString(schedule));
      json_array_append_new(timeTableArr.get(), scheduleItem.release());
    }

    json_object_set_new(node.get(), "timetable", timeTableArr.release());

    if (!stop.m_transferIds.empty())
      json_object_set_new(node.get(), "transfer_ids", IdListToJson(stop.m_transferIds).release());

    WriteJson(node.get(), stream);
  }
}

void Edges::Write(IdEdgeSet const & ids, std::ofstream & stream) const
{
  for (auto const & edgeId : ids)
  {
    auto const & edge = m_data.find(edgeId)->second;
    auto node = base::NewJSONObject();
    ToJSONObject(*node, "line_id", edgeId.m_lineId);
    ToJSONObject(*node, "stop_id_from", edgeId.m_fromStopId);
    ToJSONObject(*node, "stop_id_to", edgeId.m_toStopId);

    CHECK_GREATER(edge.m_weight, 0, (edgeId.m_fromStopId, edgeId.m_toStopId, edgeId.m_lineId));
    ToJSONObject(*node, "weight", edge.m_weight);

    json_object_set_new(node.get(), "shape", ShapeLinkToJson(edge.m_shapeLink).release());

    WriteJson(node.get(), stream);
  }
}

void EdgesTransfer::Write(IdEdgeTransferSet const & ids, std::ofstream & stream) const
{
  for (auto const & edgeTransferId : ids)
  {
    auto const weight = m_data.find(edgeTransferId)->second;
    auto node = base::NewJSONObject();

    ToJSONObject(*node, "stop_id_from", edgeTransferId.m_fromStopId);
    ToJSONObject(*node, "stop_id_to", edgeTransferId.m_toStopId);
    ToJSONObject(*node, "weight", weight);

    WriteJson(node.get(), stream);
  }
}

void Transfers::Write(IdSet const & ids, std::ofstream & stream) const
{
  for (auto transferId : ids)
  {
    auto const & transfer = m_data.find(transferId)->second;

    auto node = base::NewJSONObject();
    ToJSONObject(*node, "id", transferId);
    json_object_set_new(node.get(), "point", PointToJson(transfer.m_point).release());
    json_object_set_new(node.get(), "stops_ids", IdListToJson(transfer.m_stopsIds).release());

    WriteJson(node.get(), stream);
  }
}

void Gates::Write(IdSet const & ids, std::ofstream & stream) const
{
  for (auto gateId : ids)
  {
    auto const & gate = m_data.find(gateId)->second;
    if (gate.m_weights.empty())
      continue;

    auto node = base::NewJSONObject();
    if (gate.m_osmId == 0)
      ToJSONObject(*node, "id", gateId);
    else
      ToJSONObject(*node, "osm_id", gate.m_osmId);

    auto weightsArr = base::NewJSONArray();

    for (auto const & weight : gate.m_weights)
    {
      auto weightJson = base::NewJSONObject();
      ToJSONObject(*weightJson, "stop_id", weight.m_stopId);
      ToJSONObject(*weightJson, "time_to_stop", weight.m_timeSeconds);
      json_array_append_new(weightsArr.get(), weightJson.release());
    }

    json_object_set_new(node.get(), "weights", weightsArr.release());
    ToJSONObject(*node, "exit", gate.m_isExit);
    ToJSONObject(*node, "entrance", gate.m_isEntrance);

    json_object_set_new(node.get(), "point", PointToJson(gate.m_point).release());

    WriteJson(node.get(), stream);
  }
}

void WorldFeed::SplitFeedIntoRegions()
{
  SplitStopsBasedData();
  LOG(LINFO, ("Split stops into", m_splitting.m_stops.size(), "regions."));
  SplitLinesBasedData();
  SplitSupplementalData();
}

void WorldFeed::SplitStopsBasedData()
{
  // Fill regional stops from edges.
  for (auto const & [edgeId, data] : m_edges.m_data)
  {
    auto const [regFrom, regTo] = ExtendRegionsByPair(edgeId.m_fromStopId, edgeId.m_toStopId);
    AddToRegions(m_splitting.m_edges, edgeId, regFrom);
    AddToRegions(m_splitting.m_edges, edgeId, regTo);
  }

  // Fill regional stops from transfer edges.
  for (auto const & [edgeTransferId, data] : m_edgesTransfers.m_data)
  {
    auto const [regFrom, regTo] =
        ExtendRegionsByPair(edgeTransferId.m_fromStopId, edgeTransferId.m_toStopId);
    AddToRegions(m_splitting.m_edgesTransfers, edgeTransferId, regFrom);
    AddToRegions(m_splitting.m_edgesTransfers, edgeTransferId, regTo);
  }
}

void WorldFeed::SplitLinesBasedData()
{
  // Fill regional lines and corresponding shapes and routes.
  for (auto const & [lineId, lineData] : m_lines.m_data)
  {
    for (auto stopId : lineData.m_stopIds)
    {
      for (auto const & [region, stopIds] : m_splitting.m_stops)
      {
        if (stopIds.find(stopId) == stopIds.end())
          continue;

        m_splitting.m_lines[region][lineId].emplace_back(stopId);
        m_splitting.m_shapes[region].emplace(lineData.m_shapeLink.m_shapeId);
        m_splitting.m_routes[region].emplace(lineData.m_routeId);
      }
    }
  }

  // Fill regional networks based on routes.
  for (auto const & [region, routeIds] : m_splitting.m_routes)
  {
    for (auto routeId : routeIds)
      m_splitting.m_networks[region].emplace(m_routes.m_data[routeId].m_networkId);
  }
}

void WorldFeed::SplitSupplementalData()
{
  for (auto const & [gateId, gateData] : m_gates.m_data)
  {
    for (auto const & weight : gateData.m_weights)
      AddToRegionsIfMatches(m_splitting.m_gates, gateId, m_splitting.m_stops, weight.m_stopId);
  }

  for (auto const & [transferId, transferData] : m_transfers.m_data)
  {
    for (auto const & stopId : transferData.m_stopsIds)
      AddToRegionsIfMatches(m_splitting.m_transfers, transferId, m_splitting.m_stops, stopId);
  }
}

std::pair<Regions, Regions> WorldFeed::ExtendRegionsByPair(TransitId fromId, TransitId toId)
{
  auto const & pointFrom = m_stops.m_data[fromId].m_point;
  auto const & pointTo = m_stops.m_data[toId].m_point;
  auto const regionsFrom = AddToRegions(m_splitting.m_stops, m_affiliation, fromId, pointFrom);
  auto const regionsTo = AddToRegions(m_splitting.m_stops, m_affiliation, toId, pointTo);

  AddToRegions(m_splitting.m_stops, toId, regionsFrom);
  AddToRegions(m_splitting.m_stops, fromId, regionsTo);
  return {regionsFrom, regionsTo};
}

void WorldFeed::SaveRegions(std::string const & worldFeedDir, std::string const & region,
                            bool overwrite)
{
  auto const path = base::JoinPath(worldFeedDir, region);
  CHECK(Platform::MkDirRecursively(path), (path));

  CHECK(DumpData(m_networks, m_splitting.m_networks[region], base::JoinPath(path, kNetworksFile),
                 overwrite),
        ());
  CHECK(DumpData(m_routes, m_splitting.m_routes[region], base::JoinPath(path, kRoutesFile),
                 overwrite),
        ());
  CHECK(DumpData(m_lines, m_splitting.m_lines[region], base::JoinPath(path, kLinesFile), overwrite),
        ());
  CHECK(DumpData(m_shapes, m_splitting.m_shapes[region], base::JoinPath(path, kShapesFile),
                 overwrite),
        ());
  CHECK(DumpData(m_stops, m_splitting.m_stops[region], base::JoinPath(path, kStopsFile), overwrite),
        ());
  CHECK(DumpData(m_edges, m_splitting.m_edges[region], base::JoinPath(path, kEdgesFile), overwrite),
        ());
  CHECK(DumpData(m_edgesTransfers, m_splitting.m_edgesTransfers[region],
                 base::JoinPath(path, kEdgesTransferFile), overwrite),
        ());
  CHECK(DumpData(m_transfers, m_splitting.m_transfers[region], base::JoinPath(path, kTransfersFile),
                 overwrite),
        ());
  CHECK(DumpData(m_gates, m_splitting.m_gates[region], base::JoinPath(path, kGatesFile), overwrite),
        ());
}

bool WorldFeed::Save(std::string const & worldFeedDir, bool overwrite)
{
  CHECK(!worldFeedDir.empty(), ());
  CHECK(!m_networks.m_data.empty(), ());

  if (m_routes.m_data.empty() || m_lines.m_data.empty() || m_stops.m_data.empty() ||
      m_shapes.m_data.empty())
  {
    LOG(LINFO, ("Skipping feed with routes, lines, stops, shapes sizes:", m_routes.m_data.size(),
                m_lines.m_data.size(), m_stops.m_data.size(), m_shapes.m_data.size()));
    return false;
  }

  CHECK(!m_edges.m_data.empty(), ());
  LOG(LINFO, ("Started splitting feed into regions."));
  SplitFeedIntoRegions();
  LOG(LINFO, ("Finished splitting feed into regions. Saving to", worldFeedDir));

  for (auto const & regionAndData : m_splitting.m_stops)
    SaveRegions(worldFeedDir, regionAndData.first, overwrite);

  return true;
}

void LinkTransferIdToStop(StopData & stop, TransitId transferId)
{
  // We use vector instead of unordered set because we assume that transfers count for stop doesn't
  // exceed 2 or maybe 4.
  if (std::find(stop.m_transferIds.begin(), stop.m_transferIds.end(), transferId) ==
      stop.m_transferIds.end())
  {
    stop.m_transferIds.push_back(transferId);
  }
}
}  // namespace transit

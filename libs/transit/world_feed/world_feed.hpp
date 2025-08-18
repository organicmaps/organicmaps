#pragma once

#include "generator/affiliation.hpp"

#include "transit/transit_entities.hpp"
#include "transit/transit_schedule.hpp"
#include "transit/world_feed/color_picker.hpp"
#include "transit/world_feed/feed_helpers.hpp"

#include "geometry/mercator.hpp"
#include "geometry/point2d.hpp"

#include "defines.hpp"

#include <cstdint>
#include <fstream>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "3party/just_gtfs/just_gtfs.h"

namespace transit
{
static std::string const kDelimiter = "_";
// Generates globally unique TransitIds mapped to the GTFS entities hashes.
class IdGenerator
{
public:
  IdGenerator() = default;
  explicit IdGenerator(std::string const & idMappingPath);
  void Save();

  TransitId MakeId(std::string const & hash);

private:
  std::unordered_map<std::string, TransitId> m_hashToId;
  TransitId m_curId = 0;
  std::string m_idMappingPath;
};

// Here are OMaps representations for GTFS entities, e.g. networks for GTFS agencies.
// https://developers.google.com/transit/gtfs/reference

struct Networks
{
  void Write(IdSet const & ids, std::ofstream & stream) const;

  // Id to agency name mapping.
  std::unordered_map<TransitId, std::string> m_data;
};

struct RouteData
{
  TransitId m_networkId = 0;
  std::string m_routeType;
  std::string m_title;
  std::string m_color;
};

struct Routes
{
  void Write(IdSet const & ids, std::ofstream & stream) const;

  std::unordered_map<TransitId, RouteData> m_data;
};

struct LineData
{
  TransitId m_routeId = 0;
  ShapeLink m_shapeLink;
  std::string m_title;
  // Sequence of stops along the line from first to last.
  IdList m_stopIds;

  // Monthdays and weekdays ranges on which the line is at service.
  // Exceptions in service schedule. Explicitly activates or disables service by dates.
  // Transport intervals depending on the day timespans.
  Schedule m_schedule;

  // Fields not intended to be exported to json.
  TransitId m_shapeId = 0;
  std::string m_gtfsTripId;
  std::unordered_set<std::string> m_gtfsServiceIds;
};

struct LineSegmentInRegion
{
  // Stops in the region.
  IdList m_stopIds;
  // Spline line id to its parent line id mapping.
  std::optional<TransitId> m_splineParent = std::nullopt;
  // Indexes of the line shape link may differ in different regions.
  ShapeLink m_shapeLink;
};

struct Lines
{
  void Write(std::unordered_map<TransitId, LineSegmentInRegion> const & ids, std::ofstream & stream) const;

  std::unordered_map<TransitId, LineData> m_data;
};

struct LinesMetadata
{
  void Write(std::unordered_map<TransitId, LineSegmentInRegion> const & linesInRegion, std::ofstream & stream) const;

  // Line id to line additional data (e.g. for rendering).
  std::unordered_map<TransitId, LineSegmentsOrder> m_data;
};

struct ShapeData
{
  ShapeData() = default;
  explicit ShapeData(std::vector<m2::PointD> const & points);

  std::vector<m2::PointD> m_points;
  // Field not for dumping to json:
  IdSet m_lineIds;
};

using ShapesIter = std::unordered_map<TransitId, ShapeData>::iterator;

struct Shapes
{
  void Write(IdSet const & ids, std::ofstream & stream) const;

  std::unordered_map<TransitId, ShapeData> m_data;
};

struct StopData
{
  void UpdateTimetable(TransitId lineId, gtfs::StopTime const & stopTime);

  m2::PointD m_point;
  std::string m_title;
  // If arrival time at a specific stop for a specific trip on a route is not available,
  // |m_timetable| can be left empty.
  TimeTable m_timetable;

  // Ids of transfer nodes containing this stop.
  IdList m_transferIds;

  uint64_t m_osmId = 0;
  uint32_t m_featureId = 0;

  // Field not intended for dumping to json:
  std::string m_gtfsParentId;
};

struct Stops
{
  void Write(IdSet const & ids, std::ofstream & stream) const;

  std::unordered_map<TransitId, StopData> m_data;
};

struct Edges
{
  void Write(IdEdgeSet const & ids, std::ofstream & stream) const;

  std::unordered_map<EdgeId, EdgeData, EdgeIdHasher> m_data;
};

struct EdgeTransferId
{
  EdgeTransferId() = default;
  EdgeTransferId(TransitId fromStopId, TransitId toStopId);

  bool operator==(EdgeTransferId const & other) const;

  TransitId m_fromStopId = 0;
  TransitId m_toStopId = 0;
};

struct EdgeTransferIdHasher
{
  size_t operator()(EdgeTransferId const & key) const;
};

using IdEdgeTransferSet = std::unordered_set<EdgeTransferId, EdgeTransferIdHasher>;

struct EdgesTransfer
{
  void Write(IdEdgeTransferSet const & ids, std::ofstream & stream) const;
  // Key is pair of stops and value is |EdgeData|, containing weight (in seconds).
  std::unordered_map<EdgeTransferId, EdgeData, EdgeTransferIdHasher> m_data;
};

struct TransferData
{
  m2::PointD m_point;
  IdList m_stopsIds;
};

struct Transfers
{
  void Write(IdSet const & ids, std::ofstream & stream) const;

  std::unordered_map<TransitId, TransferData> m_data;
};

struct GateData
{
  bool m_isEntrance = false;
  bool m_isExit = false;
  m2::PointD m_point;
  std::vector<TimeFromGateToStop> m_weights;

  uint64_t m_osmId = 0;
  // Field not intended for dumping to json:
  std::string m_gtfsId;
};

struct Gates
{
  void Write(IdSet const & ids, std::ofstream & stream) const;

  std::unordered_map<TransitId, GateData> m_data;
};

// Indexes for WorldFeed |m_gtfsIdToHash| field. For each type of GTFS entity, e.g. agency or stop,
// there is distinct mapping located by its own |FieldIdx| index in the |m_gtfsIdToHash|.
enum FieldIdx
{
  AgencyIdx = 0,
  StopsIdx,
  RoutesIdx,
  TripsIdx,
  ShapesIdx,
  IdxCount
};

using GtfsIdToHash = std::unordered_map<std::string, std::string>;

struct StopsOnLines
{
  explicit StopsOnLines(IdList const & ids);

  IdList m_stopSeq;
  IdSet m_lines;
  bool m_isValid = true;
  transit::Direction m_direction = Direction::Forward;
};

using IdsInRegion = std::unordered_map<std::string, IdSet>;
using LinesInRegion = std::unordered_map<std::string, std::unordered_map<TransitId, LineSegmentInRegion>>;
using EdgeIdsInRegion = std::unordered_map<std::string, IdEdgeSet>;
using EdgeTransferIdsInRegion = std::unordered_map<std::string, IdEdgeTransferSet>;

using Regions = std::vector<std::string>;

struct TransitByRegion
{
  IdsInRegion m_networks;
  IdsInRegion m_routes;
  LinesInRegion m_lines;
  IdsInRegion m_shapes;
  IdsInRegion m_stops;
  EdgeIdsInRegion m_edges;
  EdgeTransferIdsInRegion m_edgesTransfers;
  IdsInRegion m_transfers;
  IdsInRegion m_gates;
};

// Pair of points representing corresponding edge endings.
using EdgePoints = std::pair<m2::PointD, m2::PointD>;

// Class for merging scattered GTFS feeds into one World feed with static ids.
// The usage scenario consists of steps:
// 1) Initialize |WorldFeed| instance with |IdGenerator| for correct id assignment to GTFS entities,
// |ColorPicker| for choosing route colors from our palette, |CountriesFilesAffiliation| for
// splitting result feed into regions.
// 2) Call SetFeed(...) method to convert GTFS entities into objects convenient for dumping to json.
// 3) Call Save(...) to save result data as a set of json files in the specified directory.
class WorldFeed
{
public:
  WorldFeed(IdGenerator & generator, IdGenerator & generatorEdges, ColorPicker & colorPicker,
            feature::CountriesFilesAffiliation & mwmMatcher);
  // Transforms GTFS feed into the global feed.
  bool SetFeed(gtfs::Feed && feed);

  // Dumps global feed to |world_feed_path|.
  bool Save(std::string const & worldFeedDir, bool overwrite);

private:
  friend class WorldFeedIntegrationTests;
  friend class SubwayConverterTests;
  friend class SubwayConverter;

  void SaveRegions(std::string const & worldFeedDir, std::string const & region, bool overwrite);

  bool SetFeedLanguage();
  // Fills networks from GTFS agencies data.
  bool FillNetworks();
  // Fills routes from GTFS routes data.
  bool FillRoutes();
  // Fills lines and corresponding shapes from GTFS trips and shapes.
  bool FillLinesAndShapes();
  // Deletes shapes which are sub-shapes and refreshes corresponding links in lines.
  void ModifyLinesAndShapes();
  // Gets service monthday open/closed ranges, weekdays and exceptions in schedule.
  void FillLinesSchedule();
  // Gets frequencies of trips from GTFS.

  // Adds shape with mercator points instead of WGS84 lat/lon.
  void AddShape(GtfsIdToHash::iterator & iter, gtfs::Shape const & shapeItems, TransitId lineId);
  // Fills stops data, corresponding fields in |m_lines| and builds edges for the road graph.
  bool FillStopsEdges();

  // Generates globally unique id and hash for the stop by its |stopGtfsId|.
  std::pair<TransitId, std::string> GetStopIdAndHash(std::string const & stopGtfsId);

  // Adds new stop with |stopId| and fills it with GTFS data by |gtfsId| or just
  // links to it |lineId|.
  bool UpdateStop(TransitId stopId, gtfs::StopTime const & stopTime, std::string const & stopHash, TransitId lineId);

  std::optional<TransitId> GetParentLineForSpline(TransitId lineId) const;
  bool PrepareEdgesInRegion(std::string const & region);

  TransitId GetSplineParent(TransitId lineId, std::string const & region) const;

  std::unordered_map<TransitId, std::vector<StopsOnLines>> GetStopsForShapeMatching();

  // Adds stops projections to shapes. Updates corresponding links to shapes. Returns number of
  // invalid and valid shapes.
  std::pair<size_t, size_t> ModifyShapes();
  // Fills transfers based on GTFS transfers.
  void FillTransfers();
  // Fills gates based on GTFS stops.
  void FillGates();
  // Recalculates 0-weights of edges based on the shape length.
  bool UpdateEdgeWeights();

  std::optional<Direction> ProjectStopsToShape(ShapesIter & itShape, StopsOnLines const & stopsOnLines,
                                               std::unordered_map<TransitId, std::vector<size_t>> & stopsToIndexes);

  // Splits data into regions.
  void SplitFeedIntoRegions();
  // Splits |m_stops|, |m_edges| and |m_edgesTransfer| into regions. The following stops are
  // added to corresponding regions: stops inside borders; stops which are connected with an edge
  // from |m_edges| or |m_edgesTransfer| with stops inside borders. Edge from |m_edges| or
  // |m_edgesTransfer| is added to the region if one of its stop ids lies inside mwm.
  void SplitStopsBasedData();
  // Splits |m_lines|, |m_shapes|, |m_routes| and |m_networks| into regions. If one of the line
  // stops (|m_stopIds|) lies inside region, then this line is added to this region. But only stops
  // whose stop ids are contained in this region will be attached to the line in this region. Shape,
  // route or network is added to the region if it is linked to the line in this region.
  void SplitLinesBasedData();
  // Splits |m_transfers| and |m_gates| into regions. Transfer is added to the region if there is
  // stop in |m_stopsIds| which is inside this region. Gate is added to the region if there is stop
  // in |m_weights| which is inside the region.
  void SplitSupplementalData();
  // Extend existing ids containers by appending to them |fromId| and |toId|. If one of the ids is
  // present in region, then the other is also added.
  std::pair<Regions, Regions> ExtendRegionsByPair(TransitId fromId, TransitId toId);
  // Returns true if edge weight between two stops (stop ids are contained in |edgeId|)
  // contradicts maximum transit speed.
  bool SpeedExceedsMaxVal(EdgeId const & edgeId, EdgeData const & edgeData);
  // Removes entities from feed which are linked only to the |corruptedLineIds|.
  bool ClearFeedByLineIds(std::unordered_set<TransitId> const & corruptedLineIds);
  // Current GTFS feed which is being merged to the global feed.
  gtfs::Feed m_feed;

  // Entities for json'izing and feeding to the generator_tool (Not split by regions).
  Networks m_networks;
  Routes m_routes;
  Lines m_lines;
  LinesMetadata m_linesMetadata;
  Shapes m_shapes;
  Stops m_stops;
  Edges m_edges;
  EdgesTransfer m_edgesTransfers;
  Transfers m_transfers;
  Gates m_gates;

  // Mapping of the edge to its points on the shape polyline.
  std::unordered_map<EdgeId, std::vector<std::vector<m2::PointD>>, EdgeIdHasher> m_edgesOnShapes;

  // Ids of entities for json'izing, split by regions.
  TransitByRegion m_splitting;

  // Generator of ids, globally unique and constant between re-runs.
  IdGenerator & m_idGenerator;
  // Generator of ids for edges only.
  IdGenerator & m_idGeneratorEdges;
  // Color name picker of the nearest color for route RBG from our constant list of transfer colors.
  ColorPicker & m_colorPicker;
  // Mwm matcher for m2:Points representing stops and other entities.
  feature::CountriesFilesAffiliation & m_affiliation;

  // GTFS id -> entity hash mapping. Maps GTFS id string (unique only for current feed) to the
  // globally unique hash.
  std::vector<GtfsIdToHash> m_gtfsIdToHash;

  // Unique hash characterizing each GTFS feed.
  std::string m_gtfsHash;

  // Unique hashes of all agencies handled by WorldFeed.
  static std::unordered_set<std::string> m_agencyHashes;
  // Count of corrupted stops sequences which could not be projected to the shape polyline.
  static size_t m_badStopSeqCount;
  // Agencies which are already handled by WorldFeed and should be copied to the resulting jsons.
  std::unordered_set<std::string> m_agencySkipList;

  // If the feed explicitly specifies its language, we use its value. Otherwise set to default.
  std::string m_feedLanguage;

  bool m_feedIsSplitIntoRegions = false;
};

// Creates concatenation of |values| separated by delimiter.
template <typename... Values>
auto BuildHash(Values... values)
{
  size_t constexpr paramsCount = sizeof...(Values);
  size_t const delimitersSize = (paramsCount - 1) * kDelimiter.size();
  size_t const totalSize = (delimitersSize + ... + values.size());

  std::string hash;
  hash.reserve(totalSize);
  (hash.append(values + kDelimiter), ...);
  hash.pop_back();

  return hash;
}

// Inserts |transferId| into the |stop| m_transferIds if it isn't already present there.
void LinkTransferIdToStop(StopData & stop, TransitId transferId);
}  // namespace transit

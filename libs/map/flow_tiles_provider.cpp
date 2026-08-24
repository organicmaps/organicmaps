#include "map/flow_tiles_provider.hpp"

#include "coding/mvt_reader.hpp"

#include "indexer/feature.hpp"
#include "indexer/scales.hpp"

#include "geometry/mercator.hpp"
#include "geometry/parametrized_segment.hpp"

#include "platform/http_client.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/math.hpp"

#include "routing_common/car_model.hpp"

#include <algorithm>
#include <cmath>
#include <string>
#include <utility>

using namespace std::chrono;

namespace traffic
{
namespace
{
// Vector flow tiles of the TomTom Traffic API. The tile scheme (XYZ grid,
// Mapbox Vector Tile payload, relative speeds in [0..1], road_closure tag)
// is shared by similar services; only the URL below is provider-specific.
char constexpr kFlowTilesUrl[] = "https://api.tomtom.com/traffic/map/4/tile/flow/relative/";
char constexpr kFlowLayerName[] = "Traffic flow";
auto constexpr kTileRefreshInterval = minutes(2);

size_t constexpr kMinRunVertices = 1;
size_t constexpr kFlowTilesZoom = 12;
size_t constexpr kMaxTilesPerFetch = 8;
size_t constexpr kMaxCachedTiles = 256;

double constexpr kMaxSnapDistanceMeters = 50.0;
uint32_t constexpr kMaxSegmentIdx = (1 << 15) - 1;  // TrafficInfo::RoadSegmentId::m_idx is a 15-bit field.

uint64_t TileKey(int zoom, int x, int y)
{
  return (static_cast<uint64_t>(zoom) << 50) | (static_cast<uint64_t>(x) << 25) | static_cast<uint64_t>(y);
}

// Web-mercator tile geometry over OM's map space: x equals longitude in
// degrees, y is the Mercator Y scaled to [-180; 180] and grows northwards.
m2::RectD TileRect(int zoom, int x, int y)
{
  double const size = 360.0 / (1 << zoom);
  double const minX = mercator::Bounds::kMinX + x * size;
  double const maxY = mercator::Bounds::kMaxY - y * size;
  return m2::RectD(minX, maxY - size, minX + size, maxY);
}

struct TileRange
{
  int m_minX = 0;
  int m_minY = 0;
  int m_maxX = 0;
  int m_maxY = 0;
};

TileRange TilesCovering(m2::RectD const & mercatorRect, int zoom)
{
  double const n = 1 << zoom;
  auto const toTileX = [n](double mx)
  { return static_cast<int>(math::Clamp(std::floor((mx - mercator::Bounds::kMinX) * n / 360.0), 0.0, n - 1.0)); };
  auto const toTileY = [n](double my)
  { return static_cast<int>(math::Clamp(std::floor((mercator::Bounds::kMaxY - my) * n / 360.0), 0.0, n - 1.0)); };
  TileRange r;
  r.m_minX = static_cast<int>(toTileX(mercatorRect.minX()));
  r.m_minY = static_cast<int>(toTileY(mercatorRect.maxY()));
  r.m_maxX = static_cast<int>(toTileX(mercatorRect.maxX()));
  r.m_maxY = static_cast<int>(toTileY(mercatorRect.minY()));
  return r;
}

struct RoadFeature
{
  FeatureID m_id;
  std::vector<m2::PointD> m_points;
  m2::RectD m_bbox = {};
};

void CollectRoads(DataSource const & dataSource, m2::RectD const & rect, std::vector<RoadFeature> & roads)
{
  auto const & carModel = routing::CarModel::AllLimitsInstance();
  dataSource.ForEachInRect([&](FeatureType & ft)
  {
    feature::TypesHolder const types(ft);
    if (!carModel.IsRoad(types))
      return;
    if (ft.GetGeomType() != feature::GeomType::Line)
      return;
    ft.ParseGeometry(FeatureType::BEST_GEOMETRY);
    int const numPoints = static_cast<int>(ft.GetPointsCount());
    if (numPoints < 2)
      return;
    RoadFeature rf;
    rf.m_id = ft.GetID();
    rf.m_points.reserve(static_cast<size_t>(numPoints));
    for (int i = 0; i < numPoints; ++i)
    {
      m2::PointD const & p = ft.GetPoint(i);
      rf.m_points.push_back(p);
      rf.m_bbox.Add(p);
    }
    roads.push_back(std::move(rf));
  }, rect, scales::GetUpperScale());
}

struct MatchedVertex
{
  uint32_t m_roadIdx;
  uint32_t m_segIdx;
  double m_orientation;
};

enum class TileDownload
{
  kSuccess,
  kHttpError,
  kBadPayload
};
}  // namespace

SpeedGroup SpeedGroupFromFlow(double relativeSpeed, bool closed)
{
  if (closed)
    return SpeedGroup::TempBlock;
  if (!(relativeSpeed >= 0.0))
    return SpeedGroup::Unknown;
  return GetSpeedGroupByPercentage(math::Clamp(relativeSpeed, 0.0, 1.0) * 100.0);
}

void AddToColoring(TrafficInfo::Coloring & coloring, TrafficInfo::RoadSegmentId const & id, SpeedGroup group)
{
  auto [it, inserted] = coloring.try_emplace(id, group);
  // Closures win over speed reports from neighboring tiles or line parts.
  if (!inserted && it->second != SpeedGroup::TempBlock && group == SpeedGroup::TempBlock)
    it->second = group;
}

void MergeColoring(TrafficInfo::Coloring & dst, TrafficInfo::Coloring const & src)
{
  for (auto const & [id, group] : src)
    AddToColoring(dst, id, group);
}

namespace
{
// Downloads one flow tile, decodes it and snaps its road polylines to the
// routable segments inside |colorings| (per MWM of each matched road).
TileDownload DownloadAndMatchTile(DataSource const & dataSource, std::string const & apiKey, int zoom, int x, int y,
                                  double snapToleranceMerc, std::map<MwmSet::MwmId, TrafficInfo::Coloring> & colorings)
{
  std::string const url =
      kFlowTilesUrl + std::to_string(zoom) + "/" + std::to_string(x) + "/" + std::to_string(y) + ".pbf?key=" + apiKey;

  platform::HttpClient request(url);
  if (!request.RunHttpRequest())
  {
    LOG(LWARNING, ("Traffic flow tiles request failed:", url));
    return TileDownload::kHttpError;
  }
  if (request.ErrorCode() != 200)
  {
    LOG(LWARNING, ("Traffic flow tiles request returned HTTP", request.ErrorCode(), "for", url,
                   "body:", request.ServerResponse()));
    return TileDownload::kHttpError;
  }

  std::vector<mvt::Layer> layers;
  if (!mvt::Decode(request.ServerResponse(), layers))
  {
    LOG(LWARNING, ("Could not decode a traffic flow tile"));
    return TileDownload::kBadPayload;
  }

  m2::RectD const tileRect = TileRect(zoom, x, y);
  // Roads are collected per tile: the candidate set stays small (a city tile
  // holds hundreds of roads) so matching does not blow up on large mwms.
  m2::RectD const roadsRect = m2::Inflate(tileRect, {snapToleranceMerc, snapToleranceMerc});
  std::vector<RoadFeature> roads;
  CollectRoads(dataSource, roadsRect, roads);

  bool flowLayerFound = false;
  for (auto const & layer : layers)
  {
    if (layer.m_name != kFlowLayerName)
      continue;
    flowLayerFound = true;
    for (auto const & feature : layer.m_features)
    {
      if (feature.m_geomType != mvt::GeomType::LineString)
        continue;

      mvt::Value const * levelTag = layer.GetTag(feature, "traffic_level");
      mvt::Value const * closureTag = layer.GetTag(feature, "road_closure");
      mvt::Value const * coverageTag = layer.GetTag(feature, "traffic_road_coverage");
      bool const closed = closureTag != nullptr && closureTag->m_type == mvt::Value::Type::Bool && closureTag->m_bool;
      double const relativeSpeed = levelTag != nullptr ? levelTag->m_double : 1.0;
      // Full coverage means the whole road (one-way roads), otherwise the
      // report covers one driving side and its direction is derived
      // geometrically from the polyline orientation.
      bool const fullCoverage = coverageTag == nullptr ||
                                (coverageTag->m_type == mvt::Value::Type::String && coverageTag->m_string == "full");
      SpeedGroup const group = SpeedGroupFromFlow(relativeSpeed, closed);

      for (auto const & line : feature.m_lines)
      {
        if (line.size() < 2)
          continue;

        // Convert tile units (origin top-left, y down) to mercator.
        std::vector<m2::PointD> points;
        points.reserve(line.size());
        for (auto const & p : line)
        {
          double const u = math::Clamp(p.x / layer.m_extent, -0.5, 1.5);
          double const v = math::Clamp(p.y / layer.m_extent, -0.5, 1.5);
          points.emplace_back(tileRect.minX() + u * tileRect.SizeX(), tileRect.maxY() - v * tileRect.SizeY());
        }

        // Snap each vertex to the nearest road segment within tolerance.
        // Roads are pre-filtered by their bounding boxes: a full scan over
        // every segment of ~10k roads per vertex is far too slow on dense
        // city tiles.
        std::vector<MatchedVertex> matches;
        matches.reserve(points.size());
        for (size_t i = 0; i < points.size(); ++i)
        {
          double bestDistSq = snapToleranceMerc * snapToleranceMerc;
          m2::RectD const vertexRect(points[i].x - snapToleranceMerc, points[i].y - snapToleranceMerc,
                                     points[i].x + snapToleranceMerc, points[i].y + snapToleranceMerc);
          bool found = false;
          MatchedVertex best{0, 0, 0.0};
          for (size_t r = 0; r < roads.size(); ++r)
          {
            if (!roads[r].m_bbox.IsIntersect(vertexRect))
              continue;
            auto const & roadPoints = roads[r].m_points;
            for (size_t s = 0; s + 1 < roadPoints.size(); ++s)
            {
              m2::ParametrizedSegment<m2::PointD> const segment(roadPoints[s], roadPoints[s + 1]);
              double const distSq = segment.SquaredDistanceToPoint(points[i]);
              if (!found || distSq < bestDistSq)
              {
                m2::PointD const flowDir =
                    (i + 1 < points.size()) ? points[i + 1] - points[i] : points[i] - points[i - 1];
                m2::PointD const segDir = roadPoints[s + 1] - roadPoints[s];
                bestDistSq = distSq;
                best =
                    MatchedVertex{static_cast<uint32_t>(r), static_cast<uint32_t>(s), m2::DotProduct(flowDir, segDir)};
                found = true;
              }
            }
          }
          if (found)
            matches.push_back(best);
        }
        if (matches.empty())
          continue;

        // Consecutive matches on the same road form a run; emit one
        // coloring entry per traversed segment of the run.
        size_t runStart = 0;
        while (runStart < matches.size())
        {
          size_t runEnd = runStart + 1;
          while (runEnd < matches.size() && matches[runEnd].m_roadIdx == matches[runStart].m_roadIdx)
            ++runEnd;

          if (runEnd - runStart >= kMinRunVertices)
          {
            double orientationSum = 0.0;
            for (size_t i = runStart; i < runEnd; ++i)
              orientationSum += matches[i].m_orientation;
            auto const dir = (fullCoverage || orientationSum >= 0.0) ? TrafficInfo::RoadSegmentId::kForwardDirection
                                                                     : TrafficInfo::RoadSegmentId::kReverseDirection;
            RoadFeature const & road = roads[matches[runStart].m_roadIdx];
            for (size_t i = runStart; i < runEnd; ++i)
            {
              if (matches[i].m_segIdx > kMaxSegmentIdx)
                continue;
              TrafficInfo::Coloring & coloring = colorings[road.m_id.m_mwmId];
              AddToColoring(
                  coloring,
                  TrafficInfo::RoadSegmentId(road.m_id.m_index, static_cast<uint16_t>(matches[i].m_segIdx), dir),
                  group);
            }
          }
          runStart = runEnd;
        }
      }
    }
  }
  if (!flowLayerFound && !layers.empty())
  {
    std::string names;
    for (auto const & layer : layers)
      names += (names.empty() ? "" : ", ") + layer.m_name;
    LOG(LWARNING, ("Flow tile has no '", kFlowLayerName, "' layer; layers present:", names));
    return TileDownload::kBadPayload;
  }
  return TileDownload::kSuccess;
}
}  // namespace

FlowTilesProvider::FlowTilesProvider(DataSource const & dataSource, std::string apiKey)
  : m_dataSource(dataSource)
  , m_apiKey(std::move(apiKey))
{
  CHECK(!m_apiKey.empty(), ("A flow tiles API key must be configured"));
}

FlowTilesProvider::FetchResult FlowTilesProvider::FetchColorings(
    m2::RectD const & area, std::map<MwmSet::MwmId, TrafficInfo::Coloring> & colorings)
{
  colorings.clear();

  FetchResult result;
  if (!area.IsValid())
  {
    result.m_ok = true;
    return result;
  }
  // Tolerance converted to mercator units at the area center.
  double const metersPerUnit = mercator::DistanceOnEarth(area.Center(), area.Center() + m2::PointD(0.001, 0.0)) / 0.001;
  double const snapToleranceMerc = kMaxSnapDistanceMeters / metersPerUnit;

  // Select tiles covering the area, closest to its center first.
  TileRange const range = TilesCovering(area, kFlowTilesZoom);
  m2::PointD const center = area.Center();
  std::vector<std::pair<int, int>> tiles;
  for (int x = range.m_minX; x <= range.m_maxX; ++x)
    for (int y = range.m_minY; y <= range.m_maxY; ++y)
      tiles.emplace_back(x, y);
  std::sort(tiles.begin(), tiles.end(), [&center](auto const & a, auto const & b)
  {
    m2::PointD const da = TileRect(kFlowTilesZoom, a.first, a.second).Center() - center;
    m2::PointD const db = TileRect(kFlowTilesZoom, b.first, b.second).Center() - center;
    return m2::DotProduct(da, da) < m2::DotProduct(db, db);
  });
  if (tiles.size() > kMaxTilesPerFetch)
    tiles.resize(kMaxTilesPerFetch);

  auto const now = steady_clock::now();
  bool allDownloadsOk = true;
  size_t tilesFetched = 0;
  for (auto const & [tx, ty] : tiles)
  {
    uint64_t const key = TileKey(kFlowTilesZoom, tx, ty);
    auto entryIt = m_tileCache.find(key);
    if (entryIt == m_tileCache.end() || now - entryIt->second.m_fetchedAt >= kTileRefreshInterval)
    {
      std::map<MwmSet::MwmId, TrafficInfo::Coloring> tileColorings;
      auto const status =
          DownloadAndMatchTile(m_dataSource, m_apiKey, kFlowTilesZoom, tx, ty, snapToleranceMerc, tileColorings);
      if (status == TileDownload::kHttpError)
      {
        allDownloadsOk = false;
        // Fall through and serve the stale cached copy, if any.
      }
      else
      {
        ++tilesFetched;
        // An unparsable payload keeps the previous copy when there is one:
        // some services report quota exhaustion as HTTP 200 with a non-tile
        // body, and wiping good data on such responses would be harmful.
        if (status == TileDownload::kBadPayload && entryIt != m_tileCache.end())
        {
          entryIt->second.m_fetchedAt = now;
        }
        else
        {
          entryIt = m_tileCache.try_emplace(key).first;
          entryIt->second.m_fetchedAt = now;
          entryIt->second.m_colorings = std::move(tileColorings);
          while (m_tileCache.size() > kMaxCachedTiles)
          {
            auto const oldest =
                std::min_element(m_tileCache.begin(), m_tileCache.end(), [](auto const & a, auto const & b)
            { return a.second.m_fetchedAt < b.second.m_fetchedAt; });
            m_tileCache.erase(oldest);
          }
        }
      }
    }

    // Always union the whole viewport's worth of cached data into the result,
    // not just what this cycle downloaded: callers replace previously applied
    // colorings with the result, so partial results would wipe segments that
    // belong to still-fresh tiles.
    if (entryIt != m_tileCache.end())
      for (auto const & [mwmId, tileColoring] : entryIt->second.m_colorings)
        MergeColoring(colorings[mwmId], tileColoring);
  }

  result.m_ok = allDownloadsOk;
  result.m_hasFreshData = tilesFetched > 0;
  return result;
}

std::unique_ptr<TrafficProvider> CreateFlowTilesProvider(DataSource const & dataSource, std::string apiKey)
{
  if (apiKey.empty())
    return nullptr;
  return std::make_unique<FlowTilesProvider>(dataSource, std::move(apiKey));
}
}  // namespace traffic

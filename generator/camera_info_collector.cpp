#include "generator/camera_info_collector.hpp"

#include "routing/speed_camera_ser_des.hpp"

#include "platform/local_country_file.hpp"

#include "geometry/mercator.hpp"

#include "coding/point_coding.hpp"
#include "coding/reader.hpp"

#include "base/assert.hpp"
#include "base/checked_cast.hpp"
#include "base/logging.hpp"

#include <algorithm>
#include <limits>

namespace generator
{
CamerasInfoCollector::CamerasInfoCollector(std::string const & dataFilePath,
                                           std::string const & camerasInfoPath,
                                           std::string const & osmIdsToFeatureIdsPath)
{
  routing::OsmIdToFeatureIds osmIdToFeatureIds;
  if (!routing::ParseWaysOsmIdToFeatureIdMapping(osmIdsToFeatureIdsPath, osmIdToFeatureIds))
  {
    LOG(LCRITICAL, ("An error happened while parsing feature id to osm ids mapping from file:",
                    osmIdsToFeatureIdsPath));
  }

  if (!ParseIntermediateInfo(camerasInfoPath, osmIdToFeatureIds))
    LOG(LCRITICAL, ("Unable to parse intermediate file(", camerasInfoPath, ") about cameras info"));

  platform::LocalCountryFile file = platform::LocalCountryFile::MakeTemporary(dataFilePath);
  FrozenDataSource dataSource;
  auto registerResult = dataSource.RegisterMap(file);
  if (registerResult.second != MwmSet::RegResult::Success)
    LOG(LCRITICAL, ("Unable to RegisterMap:", dataFilePath));

  std::vector<Camera> goodCameras;
  for (auto & camera : m_cameras)
  {
    camera.ParseDirection();
    camera.FindClosestSegment(dataSource, registerResult.first);

    // Don't match camera as good if we couldn't find any way.
    if (!camera.m_data.m_ways.empty())
      goodCameras.emplace_back(camera);
  }

  m_cameras = std::move(goodCameras);
}

void CamerasInfoCollector::Serialize(FileWriter & writer) const
{
  // Some cameras have several ways related to them.
  // We create N cameras with one way attached from a camera that has N ways attached.
  std::vector<CamerasInfoCollector::Camera> flattenedCameras;
  for (auto const & camera : m_cameras)
  {
    for (auto const & way : camera.m_data.m_ways)
    {
      flattenedCameras.emplace_back(camera.m_data.m_center, camera.m_data.m_maxSpeedKmPH,
                                    std::vector<routing::SpeedCameraMwmPosition>{way});
    }
  }

  routing::SpeedCameraMwmHeader header;
  header.SetVersion(routing::SpeedCameraMwmHeader::kLatestVersion);
  header.SetAmount(base::asserted_cast<uint32_t>(flattenedCameras.size()));
  header.Serialize(writer);

  std::sort(flattenedCameras.begin(), flattenedCameras.end(), [](auto const & a, auto const & b)
  {
    CHECK_EQUAL(a.m_data.m_ways.size(), 1, ());
    CHECK_EQUAL(b.m_data.m_ways.size(), 1, ());

    auto const & aWay = a.m_data.m_ways.back();
    auto const & bWay = b.m_data.m_ways.back();

    if (aWay.m_featureId != bWay.m_featureId)
      return aWay.m_featureId < bWay.m_featureId;

    if (aWay.m_segmentId != bWay.m_segmentId)
      return aWay.m_segmentId < bWay.m_segmentId;

    return aWay.m_coef < bWay.m_coef;
  });

  // Now each camera has only 1 way.
  uint32_t prevFeatureId = 0;
  for (auto const & camera : flattenedCameras)
    camera.Serialize(writer, prevFeatureId);
}

bool CamerasInfoCollector::ParseIntermediateInfo(
    std::string const & camerasInfoPath, routing::OsmIdToFeatureIds const & osmIdToFeatureIds)
{
  FileReader reader(camerasInfoPath);
  ReaderSource<FileReader> src(reader);

  uint32_t maxSpeedKmPH = 0;
  uint32_t relatedWaysNumber = 0;

  std::vector<routing::SpeedCameraMwmPosition> ways;
  uint32_t latInt = 0;
  double lat = 0;

  uint32_t lonInt = 0;
  double lon = 0;
  m2::PointD center;

  while (src.Size() > 0)
  {
    ReadPrimitiveFromSource(src, latInt);
    ReadPrimitiveFromSource(src, lonInt);
    lat = Uint32ToDouble(latInt, ms::LatLon::kMinLat, ms::LatLon::kMaxLat, kPointCoordBits);
    lon = Uint32ToDouble(lonInt, ms::LatLon::kMinLon, ms::LatLon::kMaxLon, kPointCoordBits);

    ReadPrimitiveFromSource(src, maxSpeedKmPH);
    ReadPrimitiveFromSource(src, relatedWaysNumber);

    center = mercator::FromLatLon(lat, lon);

    if (maxSpeedKmPH >= routing::kMaxCameraSpeedKmpH)
    {
      LOG(LINFO, ("Bad SpeedCamera max speed:", maxSpeedKmPH));
      maxSpeedKmPH = 0;
    }

    bool badCamera = false;
    if (relatedWaysNumber > std::numeric_limits<uint8_t>::max())
    {
      badCamera = true;
      LOG(LERROR, ("Number of related to camera ways should be interval from 0 to 255.",
                   "lat(", lat, "), lon(", lon, ")"));
    }

    uint64_t wayOsmId = 0;
    for (uint32_t i = 0; i < relatedWaysNumber; ++i)
    {
      ReadPrimitiveFromSource(src, wayOsmId);

      auto const it = osmIdToFeatureIds.find(base::MakeOsmWay(wayOsmId));
      if (it != osmIdToFeatureIds.cend())
      {
        auto const & featureIdsVec = it->second;
        // Note. One |wayOsmId| may correspond several feature ids.
        for (auto const featureId : featureIdsVec)
          ways.emplace_back(featureId, 0 /* segmentId */, 0 /* coef */);
      }
    }

    auto const speed = base::asserted_cast<uint8_t>(maxSpeedKmPH);
    if (!badCamera)
      m_cameras.emplace_back(center, speed, std::move(ways));
  }

  return true;
}

void CamerasInfoCollector::Camera::FindClosestSegment(FrozenDataSource const & dataSource,
                                                      MwmSet::MwmId const & mwmId)
{
  if (!m_data.m_ways.empty() && FindClosestSegmentInInnerWays(dataSource, mwmId))
    return;

  FindClosestSegmentWithGeometryIndex(dataSource);
}


bool CamerasInfoCollector::Camera::FindClosestSegmentInInnerWays(FrozenDataSource const & dataSource,
                                                                 MwmSet::MwmId const & mwmId)
{
  // If m_ways is not empty. It means, that one of point it is our camera.
  // So we should find it in feature's points.
  for (auto it = m_data.m_ways.begin(); it != m_data.m_ways.end();)
  {
    if (auto id = FindMyself(it->m_featureId, dataSource, mwmId))
    {
      it->m_segmentId = (*id).second;
      it->m_coef = (*id).first;  // Camera starts at the beginning or the end of a segment.
      CHECK(it->m_coef == 0.0 || it->m_coef == 1.0, ("Coefficient must be 0.0 or 1.0 here"));
      ++it;
    }
    else
    {
      it = m_data.m_ways.erase(it);
    }
  }

  return !m_data.m_ways.empty();
}

void CamerasInfoCollector::Camera::FindClosestSegmentWithGeometryIndex(FrozenDataSource const & dataSource)
{
  uint32_t bestFeatureId = 0;
  uint32_t bestSegmentId = 0;
  auto bestMinDist = kMaxDistFromCameraToClosestSegmentMeters;
  bool found = false;
  double bestCoef = 0.0;

  // Look at each segment of roads and find the closest.
  auto const updateClosestFeatureCallback = [&](FeatureType & ft) {
    if (ft.GetGeomType() != feature::GeomType::Line)
      return;

    if (!routing::IsCarRoad(feature::TypesHolder(ft)))
      return;

    auto curMinDist = kMaxDistFromCameraToClosestSegmentMeters;

    ft.ParseGeometry(FeatureType::BEST_GEOMETRY);
    std::vector<m2::PointD> points(ft.GetPointsCount());
    for (size_t i = 0; i < points.size(); ++i)
      points[i] = ft.GetPoint(i);

    routing::FollowedPolyline polyline(points.begin(), points.end());
    m2::RectD const rect = mercator::RectByCenterXYAndSizeInMeters(m_data.m_center, kSearchCameraRadiusMeters);
    auto const curSegment = polyline.UpdateProjection(rect);
    if (!curSegment.IsValid())
      return;

    CHECK_LESS(curSegment.m_ind + 1, polyline.GetPolyline().GetSize(), ());
    static double constexpr kEps = 1e-6;
    auto const & p1 = polyline.GetPolyline().GetPoint(curSegment.m_ind);
    auto const & p2 = polyline.GetPolyline().GetPoint(curSegment.m_ind + 1);

    if (AlmostEqualAbs(p1, p2, kEps))
      return;

    m2::ParametrizedSegment<m2::PointD> st(p1, p2);
    auto const cameraProjOnSegment = st.ClosestPointTo(m_data.m_center);
    curMinDist = mercator::DistanceOnEarth(cameraProjOnSegment, m_data.m_center);

    if (curMinDist < bestMinDist)
    {
      bestMinDist = curMinDist;
      bestFeatureId = ft.GetID().m_index;
      bestSegmentId = static_cast<uint32_t>(curSegment.m_ind);
      bestCoef = mercator::DistanceOnEarth(p1, cameraProjOnSegment) / mercator::DistanceOnEarth(p1, p2);
      found = true;
    }
  };

  dataSource.ForEachInRect(
    updateClosestFeatureCallback,
    mercator::RectByCenterXYAndSizeInMeters(m_data.m_center, kSearchCameraRadiusMeters),
    scales::GetUpperScale());

  if (found)
    m_data.m_ways.emplace_back(bestFeatureId, bestSegmentId, bestCoef);
}

std::optional<std::pair<double, uint32_t>> CamerasInfoCollector::Camera::FindMyself(
    uint32_t wayFeatureId, FrozenDataSource const & dataSource, MwmSet::MwmId const & mwmId) const
{
  double coef = 0.0;
  bool isRoad = true;
  uint32_t result = 0;
  bool cannotFindMyself = false;

  auto const readFeature = [&](FeatureType & ft) {
    bool found = false;
    isRoad = routing::IsRoad(feature::TypesHolder(ft));
    if (!isRoad)
      return;

    auto const findPoint = [&result, &found, this](m2::PointD const & pt) {
      if (found)
        return;

      if (AlmostEqualAbs(m_data.m_center, pt, kCoordEqualityEps))
        found = true;
      else
        ++result;
    };

    ft.ForEachPoint(findPoint, scales::GetUpperScale());

    if (!found)
    {
      cannotFindMyself = true;
      return;
    }

    // If point with number - N, is end of feature, we cannot say: segmentId = N,
    // because number of segments is N - 1, so we say segmentId = N - 1, and coef = 1
    // which means, that camera placed at the end of (N - 1)'th segment.
    if (result + 1 == ft.GetPointsCount())
    {
      CHECK_NOT_EQUAL(result, 0, ("Feature consists of one point!"));
      --result;
      coef = 1.0;
    }
  };

  FeatureID featureID(mwmId, wayFeatureId);
  dataSource.ReadFeature(readFeature, featureID);

  if (cannotFindMyself)
  {
    // Note. Speed camera with coords |m_data.m_center| is not located at any point of |wayFeatureId|.
    // It may happens if osm feature corresponds to |wayFeatureId| is split by a mini_roundabout
    // or turning_loop. There are two cases:
    // * |m_data.m_center| is located at a point of another part of the whole osm feature. In
    //   that case it will be found on another call of FindMyself() method.
    // * |m_data.m_center| coincides with point of mini_roundabout or turning_loop. It means
    //   there on feature point which coincides with speed camera (|m_data.m_center|).
    //   These camera (notification) will be lost.
    return {};
  }

  if (isRoad)
    return std::optional<std::pair<double, uint32_t>>({coef, result});

  return {};
}

void CamerasInfoCollector::Camera::Serialize(FileWriter & writer,
                                             uint32_t & prevFeatureId) const
{
  routing::SerializeSpeedCamera(writer, m_data, prevFeatureId);
}

void BuildCamerasInfo(std::string const & dataFilePath,
                      std::string const & camerasInfoPath,
                      std::string const & osmIdsToFeatureIdsPath)
{
  LOG(LINFO, ("Generating cameras info for", dataFilePath));

  generator::CamerasInfoCollector collector(dataFilePath, camerasInfoPath, osmIdsToFeatureIdsPath);

  FilesContainerW cont(dataFilePath, FileWriter::OP_WRITE_EXISTING);
  auto writer = cont.GetWriter(CAMERAS_INFO_FILE_TAG);

  collector.Serialize(*writer);
}
}  // namespace generator

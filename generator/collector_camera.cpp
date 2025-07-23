#include "generator/collector_camera.hpp"

#include "generator/feature_builder.hpp"
#include "generator/intermediate_data.hpp"
#include "generator/maxspeeds_parser.hpp"
#include "generator/osm_element.hpp"

#include "routing/routing_helpers.hpp"

#include "routing_common/maxspeed_conversion.hpp"

#include "indexer/ftypes_matcher.hpp"

#include "platform/measurement_utils.hpp"
#include "platform/platform.hpp"

#include "coding/point_coding.hpp"
#include "coding/reader.hpp"
#include "coding/write_to_sink.hpp"
#include "coding/writer.hpp"

#include "geometry/latlon.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

#include <algorithm>

namespace routing_builder
{

std::optional<double> GetMaxSpeedKmPH(std::string const & maxSpeedString)
{
  routing::SpeedInUnits speed;
  if (!generator::ParseMaxspeedTag(maxSpeedString, speed) || !speed.IsNumeric())
    return {};

  auto const speedKmPh = measurement_utils::ToSpeedKmPH(speed.GetSpeed(), speed.GetUnits());
  if (speedKmPh < 0.0)
    return {};

  return {speedKmPh};
}

CameraCollector::CameraInfo::CameraInfo(OsmElement const & element)
  : m_id(element.m_id)
  , m_lon(element.m_lon)
  , m_lat(element.m_lat)
{
  // Filter long "maxspeed" dummy strings.
  auto const maxspeed = element.GetTag("maxspeed");
  if (maxspeed.empty() || maxspeed.size() > 32)
    return;

  if (auto const validatedMaxspeed = GetMaxSpeedKmPH(maxspeed))
    m_speedKmPH = static_cast<uint32_t>(*validatedMaxspeed);
  else
    LOG(LWARNING, ("Bad speed format of camera:", maxspeed, ", osmId:", element.m_id));
}

// static
CameraCollector::CameraInfo CameraCollector::CameraInfo::Read(ReaderSource<FileReader> & src)
{
  /// @todo Take out intermediate camera info serialization code.
  /// Should be equal with CamerasInfoCollector::ParseIntermediateInfo.

  CameraInfo camera;
  auto const latInt = ReadPrimitiveFromSource<uint32_t>(src);
  auto const lonInt = ReadPrimitiveFromSource<uint32_t>(src);
  camera.m_lat = Uint32ToDouble(latInt, ms::LatLon::kMinLat, ms::LatLon::kMaxLat, kPointCoordBits);
  camera.m_lon = Uint32ToDouble(lonInt, ms::LatLon::kMinLon, ms::LatLon::kMaxLon, kPointCoordBits);
  ReadPrimitiveFromSource(src, camera.m_speedKmPH);
  auto relatedWaysNumber = ReadPrimitiveFromSource<uint32_t>(src);
  camera.m_ways.reserve(relatedWaysNumber);
  while (relatedWaysNumber--)
    camera.m_ways.push_back(ReadPrimitiveFromSource<uint64_t>(src));

  return camera;
}

// static
void CameraCollector::CameraInfo::Write(FileWriter & writer, CameraInfo const & camera)
{
  uint32_t const lat = DoubleToUint32(camera.m_lat, ms::LatLon::kMinLat, ms::LatLon::kMaxLat, kPointCoordBits);
  WriteToSink(writer, lat);

  uint32_t const lon = DoubleToUint32(camera.m_lon, ms::LatLon::kMinLon, ms::LatLon::kMaxLon, kPointCoordBits);
  WriteToSink(writer, lon);

  WriteToSink(writer, camera.m_speedKmPH);

  auto const size = static_cast<uint32_t>(camera.m_ways.size());
  WriteToSink(writer, size);
  for (auto wayId : camera.m_ways)
    WriteToSink(writer, wayId);
}

void CameraCollector::FillCameraInWays()
{
  for (uint64_t id : m_roadOsmIDs)
  {
    WayElement way(id);
    CHECK(m_cache->GetWay(id, way), ());

    for (auto const & node : way.m_nodes)
    {
      auto it = m_speedCameras.find(node);
      if (it != m_speedCameras.cend())
        it->second.m_ways.push_back(id);
    }
  }

  size_t detachedCameras = 0;
  ForEachCamera([&detachedCameras](auto & c)
  {
    if (c.m_ways.empty())
      ++detachedCameras;
    else
      base::SortUnique(c.m_ways);
  });

  LOG(LINFO, ("Total cameras count:", m_speedCameras.size(), "Detached cameras count:", detachedCameras));
}

void CameraCollector::MergeInto(CameraCollector & collector) const
{
  collector.m_speedCameras.insert(m_speedCameras.begin(), m_speedCameras.end());
  collector.m_roadOsmIDs.insert(collector.m_roadOsmIDs.end(), m_roadOsmIDs.begin(), m_roadOsmIDs.end());
}

void CameraCollector::Save()
{
  LOG(LINFO, ("Saving speed cameras to", GetFilename()));

  FillCameraInWays();

  FileWriter writer(GetFilename());
  ForEachCamera([&](auto const & camera) { CameraInfo::Write(writer, camera); });
}

void CameraCollector::OrderCollectedData()
{
  std::vector<CameraInfo> collectedData;
  {
    FileReader reader(GetFilename());
    ReaderSource src(reader);
    while (src.Size() > 0)
      collectedData.emplace_back(CameraInfo::Read(src));
  }

  std::sort(std::begin(collectedData), std::end(collectedData));

  FileWriter writer(GetFilename());
  for (auto const & camera : collectedData)
    CameraInfo::Write(writer, camera);
}

CameraCollector::CameraCollector(std::string const & filename, IDRInterfacePtr cache)
  : generator::CollectorInterface(filename)
  , m_cache(std::move(cache))
{}

std::shared_ptr<generator::CollectorInterface> CameraCollector::Clone(IDRInterfacePtr const & cache) const
{
  return std::make_shared<CameraCollector>(GetFilename(), cache);
}

void CameraCollector::CollectFeature(feature::FeatureBuilder const & fb, OsmElement const & element)
{
  if (element.m_type == OsmElement::EntityType::Node)
  {
    if (ftypes::IsSpeedCamChecker::Instance()(fb.GetTypes()))
      m_speedCameras.emplace(element.m_id, element);
  }
  else if (element.m_type == OsmElement::EntityType::Way)
  {
    if (fb.IsLine() && routing::IsCarRoad(fb.GetTypes()))
      m_roadOsmIDs.push_back(element.m_id);
  }
}

}  // namespace routing_builder

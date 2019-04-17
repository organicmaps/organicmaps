#include "generator/collector_camera.hpp"

#include "generator/feature_builder.hpp"
#include "generator/osm_element.hpp"
#include "generator/maxspeeds_parser.hpp"

#include "routing/routing_helpers.hpp"

#include "routing_common/maxspeed_conversion.hpp"

#include "indexer/ftypes_matcher.hpp"

#include "platform/measurement_utils.hpp"

#include "coding/point_coding.hpp"
#include "coding/write_to_sink.hpp"

#include "geometry/latlon.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/string_utils.hpp"

namespace routing
{
size_t const CameraProcessor::kMaxSpeedSpeedStringLength = 32;

std::string ValidateMaxSpeedString(std::string const & maxSpeedString)
{
  routing::SpeedInUnits speed;
  if (!generator::ParseMaxspeedTag(maxSpeedString, speed) || !speed.IsNumeric())
    return std::string();

  return strings::to_string(measurement_utils::ToSpeedKmPH(speed.GetSpeed(), speed.GetUnits()));
}

CameraProcessor::CameraInfo::CameraInfo(OsmElement const & element)
  : m_id(element.m_id)
  , m_lon(element.m_lon)
  , m_lat(element.m_lat)
{
  auto const maxspeed = element.GetTag("maxspeed");
  if (!maxspeed.empty())
    m_speed = ValidateMaxSpeedString(maxspeed);
}

void CameraProcessor::ForEachCamera(Fn && toDo) const
{
  std::vector<uint64_t> empty;
  for (auto const & p : m_speedCameras)
  {
    auto const & ways = m_cameraToWays.count(p.first) != 0 ? m_cameraToWays.at(p.first) : empty;
    toDo(p.second, ways);
  }
}

void CameraProcessor::ProcessWay(OsmElement const & element)
{
  for (auto const node : element.m_nodes)
  {
    if (m_speedCameras.find(node) == m_speedCameras.cend())
      continue;

    auto & ways = m_cameraToWays[node];
    ways.push_back(element.m_id);
  }
}

void CameraProcessor::ProcessNode(OsmElement const & element)
{
  CameraInfo camera(element);
  CHECK_LESS(camera.m_speed.size(), kMaxSpeedSpeedStringLength, ());
  m_speedCameras.emplace(element.m_id, std::move(camera));
}

CameraCollector::CameraCollector(std::string const & writerFile) :
  m_fileWriter(writerFile) {}

void CameraCollector::CollectFeature(FeatureBuilder1 const & feature, OsmElement const & element)
{
  switch (element.m_type)
  {
  case OsmElement::EntityType::Node:
  {
    if (ftypes::IsSpeedCamChecker::Instance()(feature.GetTypes()))
      m_processor.ProcessNode(element);
    break;
  }
  case OsmElement::EntityType::Way:
  {
    if (routing::IsCarRoad(feature.GetTypes()))
      m_processor.ProcessWay(element);
    break;
  }
  default:
    break;
  }
}

void CameraCollector::Write(CameraProcessor::CameraInfo const & camera, std::vector<uint64_t> const & ways)
{
  std::string maxSpeedStringKmPH = camera.m_speed;
  int32_t maxSpeedKmPH = 0;
  if (!strings::to_int(maxSpeedStringKmPH.c_str(), maxSpeedKmPH))
    LOG(LWARNING, ("Bad speed format of camera:", maxSpeedStringKmPH, ", osmId:", camera.m_id));

  CHECK_GREATER_OR_EQUAL(maxSpeedKmPH, 0, ());

  uint32_t const lat =
      DoubleToUint32(camera.m_lat, ms::LatLon::kMinLat, ms::LatLon::kMaxLat, kPointCoordBits);
  WriteToSink(m_fileWriter, lat);

  uint32_t const lon =
      DoubleToUint32(camera.m_lon, ms::LatLon::kMinLon, ms::LatLon::kMaxLon, kPointCoordBits);
  WriteToSink(m_fileWriter, lon);

  WriteToSink(m_fileWriter, static_cast<uint32_t>(maxSpeedKmPH));

  auto const size = static_cast<uint32_t>(ways.size());
  WriteToSink(m_fileWriter, size);
  for (auto wayId : ways)
    WriteToSink(m_fileWriter, wayId);
}

void CameraCollector::Save()
{
  using namespace std::placeholders;
  m_processor.ForEachCamera(std::bind(&CameraCollector::Write, this, _1, _2));
}
}  // namespace routing

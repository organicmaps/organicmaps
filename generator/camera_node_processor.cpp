#include "generator/camera_node_processor.hpp"

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
  for (auto const node : element.m_nds)
  {
    if (m_speedCameras.find(node) == m_speedCameras.cend())
      continue;

    auto & ways = m_cameraToWays[node];
    ways.push_back(element.id);
  }
}

// static
std::string CameraProcessor::ValidateMaxSpeedString(std::string const & maxSpeedString)
{
  routing::SpeedInUnits speed;
  if (!generator::ParseMaxspeedTag(maxSpeedString, speed) || !speed.IsNumeric())
    return std::string();

  return strings::to_string(measurement_utils::ToSpeedKmPH(speed.GetSpeed(), speed.GetUnits()));
}

void CameraProcessor::ProcessNode(OsmElement const & element)
{
  CameraInfo camera;
  camera.m_id = element.id;
  camera.m_lat = element.lat;
  camera.m_lon = element.lon;
  auto const maxspeed = element.GetTag("maxspeed");
  if (!maxspeed.empty())
    camera.m_speed = ValidateMaxSpeedString(maxspeed);

  CHECK_LESS(camera.m_speed.size(), kMaxSpeedSpeedStringLength, ("Too long string for speed"));
  m_speedCameras.emplace(element.id, std::move(camera));
}

CameraNodeProcessor::CameraNodeProcessor(std::string const & writerFile) :
  m_fileWriter(writerFile) {}

void CameraNodeProcessor::CollectFeature(FeatureBuilder1 const & feature, OsmElement const & element)
{
  switch (element.type)
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

void CameraNodeProcessor::Write(CameraProcessor::CameraInfo const & camera, std::vector<uint64_t> const & ways)
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

void CameraNodeProcessor::Save()
{
  using namespace std::placeholders;
  m_processor.ForEachCamera(std::bind(&CameraNodeProcessor::Write, this, _1, _2));
}
}  // namespace routing

#include "generator/collector_camera.hpp"

#include "generator/feature_builder.hpp"
#include "generator/intermediate_data.hpp"
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

#include <algorithm>
#include <iterator>

using namespace feature;


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
  m_ways[element.m_id] = element.m_nodes;
}

void CameraProcessor::FillCameraInWays()
{
  for (auto const & way : m_ways)
  {
    for (auto const & node : way.second)
    {
      auto const itCamera = m_speedCameras.find(node);
      if (itCamera == m_speedCameras.cend())
        continue;

      m_cameraToWays[itCamera->first].push_back(way.first);
    }
  }
}

void CameraProcessor::ProcessNode(OsmElement const & element)
{
  CameraInfo camera(element);
  CHECK_LESS(camera.m_speed.size(), kMaxSpeedSpeedStringLength, ());
  m_speedCameras.emplace(element.m_id, std::move(camera));
}

void CameraProcessor::Merge(CameraProcessor const & cameraProcessor)
{
  auto const & otherCameras = cameraProcessor.m_speedCameras;
  m_speedCameras.insert(std::begin(otherCameras), std::end(otherCameras));

  auto const & otherWays = cameraProcessor.m_ways;
  m_ways.insert(std::begin(otherWays), std::end(otherWays));
}

CameraCollector::CameraCollector(std::string const & filename) :
  generator::CollectorInterface(filename) {}

std::shared_ptr<generator::CollectorInterface>
CameraCollector::Clone(std::shared_ptr<generator::cache::IntermediateDataReader> const &) const
{
  return std::make_shared<CameraCollector>(GetFilename());
}

void CameraCollector::CollectFeature(FeatureBuilder const & feature, OsmElement const & element)
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

void CameraCollector::Write(FileWriter & writer, CameraProcessor::CameraInfo const & camera,
                            std::vector<uint64_t> const & ways)
{
  std::string maxSpeedStringKmPH = camera.m_speed;
  int32_t maxSpeedKmPH = 0;
  if (!strings::to_int(maxSpeedStringKmPH.c_str(), maxSpeedKmPH))
    LOG(LWARNING, ("Bad speed format of camera:", maxSpeedStringKmPH, ", osmId:", camera.m_id));

  CHECK_GREATER_OR_EQUAL(maxSpeedKmPH, 0, ());

  uint32_t const lat =
      DoubleToUint32(camera.m_lat, ms::LatLon::kMinLat, ms::LatLon::kMaxLat, kPointCoordBits);
  WriteToSink(writer, lat);

  uint32_t const lon =
      DoubleToUint32(camera.m_lon, ms::LatLon::kMinLon, ms::LatLon::kMaxLon, kPointCoordBits);
  WriteToSink(writer, lon);

  WriteToSink(writer, static_cast<uint32_t>(maxSpeedKmPH));

  auto const size = static_cast<uint32_t>(ways.size());
  WriteToSink(writer, size);
  for (auto wayId : ways)
    WriteToSink(writer, wayId);
}

void CameraCollector::Save()
{
  LOG(LINFO, ("void CameraCollector::Save()"));
  using namespace std::placeholders;
  m_processor.FillCameraInWays();
  FileWriter writer(GetFilename());
  m_processor.ForEachCamera(std::bind(&CameraCollector::Write, this, std::ref(writer), _1, _2));
}

void CameraCollector::Merge(generator::CollectorInterface const * collector)
{
  CHECK(collector, ());

  collector->MergeInto(const_cast<CameraCollector *>(this));
}

void CameraCollector::MergeInto(CameraCollector * collector) const
{
  CHECK(collector, ());

  collector->m_processor.Merge(this->m_processor);
}
}  // namespace routing

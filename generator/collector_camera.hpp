#pragma once

#include "generator/collector_interface.hpp"

#include "coding/file_writer.hpp"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace generator_tests
{
class TestCameraCollector;
}  // namespace generator_tests

struct OsmElement;

namespace feature
{
class FeatureBuilder;
}  // namespace feature

// TODO (@gmoryes) move members of m_routingTagsProcessor to generator
namespace routing
{
/// \brief Gets text with speed, returns formatted speed string in km per hour.
/// \param maxSpeedString - text with speed. Possible format:
///                         "130" - means 130 km per hour.
///                         "130 mph" - means 130 miles per hour.
///                         "130 kmh" - means 130 km per hour.
/// See https://wiki.openstreetmap.org/wiki/Key:maxspeed
/// for more details about input string.
std::string ValidateMaxSpeedString(std::string const & maxSpeedString);

class CameraProcessor
{
public:
  struct CameraInfo;
  using Fn = std::function<void (CameraInfo const &, std::vector<uint64_t> const &)>;

  struct CameraInfo
  {
    CameraInfo(const OsmElement & element);

    uint64_t m_id = 0;
    double m_lon = 0.0;
    double m_lat = 0.0;
    std::string m_speed;
    std::vector<uint64_t> m_ways;
  };

  static size_t const kMaxSpeedSpeedStringLength;

  void ForEachCamera(Fn && toDo) const;
  void ProcessNode(OsmElement const & element);
  void ProcessWay(OsmElement const & element);

private:
  std::unordered_map<uint64_t, CameraInfo> m_speedCameras;
  std::unordered_map<uint64_t, std::vector<uint64_t>> m_cameraToWays;
};

class CameraCollector : public generator::CollectorInterface
{
public:
  friend class generator_tests::TestCameraCollector;

  explicit CameraCollector(std::string const & writerFile);

  // generator::CollectorInterface overrides:
  // We will process all nodes before ways because of o5m format:
  // all nodes are first, then all ways, then all relations.
  void CollectFeature(feature::FeatureBuilder const & feature, OsmElement const & element) override;
  void Save() override;

private:
  void Write(CameraProcessor::CameraInfo const & camera, std::vector<uint64_t> const & ways);

  FileWriter m_fileWriter;
  CameraProcessor m_processor;
};
}  // namespace routing

#pragma once

#include "generator/collector_interface.hpp"

#include "coding/file_writer.hpp"

#include <cstdint>
#include <functional>
#include <fstream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace generator_tests
{
class TestCameraCollector;
}  // namespace generator_tests
namespace generator
{
namespace cache
{
class IntermediateDataReader;
}  // namespace cache
}  // namespace generator

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

    bool IsValid() const { return m_speed >= 0; }

    uint64_t m_id = 0;
    double m_lon = 0.0;
    double m_lat = 0.0;
    int32_t m_speed = 0;
    std::vector<uint64_t> m_ways;
  };

  static size_t const kMaxSpeedSpeedStringLength;

  CameraProcessor(std::string const & filename);
  ~CameraProcessor();

  void ForEachCamera(Fn && toDo) const;
  void ProcessNode(OsmElement const & element);
  void ProcessWay(OsmElement const & element);

  void FillCameraInWays();

  void Finish();
  void Merge(CameraProcessor const & cameraProcessor);

private:
  std::string m_waysFilename;
  std::unique_ptr<FileWriter> m_waysWriter;
  std::unordered_map<uint64_t, CameraInfo> m_speedCameras;
  std::unordered_map<uint64_t, std::vector<uint64_t>> m_cameraToWays;
};

class CameraCollector : public generator::CollectorInterface
{
public:
  friend class generator_tests::TestCameraCollector;

  explicit CameraCollector(std::string const & filename);

  // generator::CollectorInterface overrides:
  std::shared_ptr<CollectorInterface>
  Clone(std::shared_ptr<generator::cache::IntermediateDataReader> const & = {}) const override;
  // We will process all nodes before ways because of o5m format:
  // all nodes are first, then all ways, then all relations.
  void CollectFeature(feature::FeatureBuilder const & feature, OsmElement const & element) override;
  void Finish() override;
  void Save() override;

  void Merge(generator::CollectorInterface const & collector) override;
  void MergeInto(CameraCollector & collector) const override;

private:
  void Write(FileWriter & writer, CameraProcessor::CameraInfo const & camera,
             std::vector<uint64_t> const & ways);

  CameraProcessor m_processor;
};
}  // namespace routing

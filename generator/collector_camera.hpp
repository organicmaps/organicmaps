#pragma once

#include "generator/collector_interface.hpp"

#include <cstdint>
#include <fstream>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

template <typename T>
class ReaderSource;

class FileReader;
class FileWriter;

// TODO (@gmoryes) move members of m_routingTagsProcessor to generator
namespace routing_builder
{
class TestCameraCollector;

/// \brief Returns formatted speed in km per hour.
/// \param maxSpeedString - text with speed. Possible format:
///                         "130" - means 130 km per hour.
///                         "130 mph" - means 130 miles per hour.
///                         "130 kmh" - means 130 km per hour.
/// See https://wiki.openstreetmap.org/wiki/Key:maxspeed
/// for more details about input string.
std::optional<double> GetMaxSpeedKmPH(std::string const & maxSpeedString);

class CameraProcessor
{
public:
  struct CameraInfo;

  struct CameraInfo
  {
    CameraInfo() = default;
    explicit CameraInfo(OsmElement const & element);

    friend bool operator<(CameraInfo const & lhs, CameraInfo const & rhs)
    {
      return std::tie(lhs.m_id, lhs.m_lon, lhs.m_lat, lhs.m_speedKmPH, lhs.m_ways) <
             std::tie(rhs.m_id, rhs.m_lon, rhs.m_lat, rhs.m_speedKmPH, rhs.m_ways);
    }

    static CameraInfo Read(ReaderSource<FileReader> & src);
    static void Write(FileWriter & writer, CameraInfo const & camera);

    void Normalize();

    uint64_t m_id = 0;
    double m_lon = 0.0;
    double m_lat = 0.0;
    uint32_t m_speedKmPH = 0;
    std::vector<uint64_t> m_ways;
  };

  static size_t const kMaxSpeedSpeedStringLength;

  CameraProcessor(std::string const & filename);
  ~CameraProcessor();

  template <typename Fn>
  void ForEachCamera(Fn && toDo)
  {
    for (auto & p : m_speedCameras)
      toDo(p.second);
  }

  void ProcessNode(OsmElement const & element);
  void ProcessWay(OsmElement const & element);

  void FillCameraInWays();

  void Finish();
  void Merge(CameraProcessor const & cameraProcessor);
  void Save(std::string const & filename);
  void OrderCollectedData(std::string const & filename);

private:
  std::string m_waysFilename;
  std::unique_ptr<FileWriter> m_waysWriter;
  std::unordered_map<uint64_t, CameraInfo> m_speedCameras;
  std::unordered_map<uint64_t, std::vector<uint64_t>> m_cameraToWays;
};

class CameraCollector : public generator::CollectorInterface
{
  friend class TestCameraCollector;

public:
  explicit CameraCollector(std::string const & filename);

  // generator::CollectorInterface overrides:
  std::shared_ptr<CollectorInterface> Clone(
      std::shared_ptr<generator::cache::IntermediateDataReaderInterface> const & = {})
      const override;
  // We will process all nodes before ways because of o5m format:
  // all nodes are first, then all ways, then all relations.
  void CollectFeature(feature::FeatureBuilder const & feature, OsmElement const & element) override;
  void Finish() override;

  void Merge(generator::CollectorInterface const & collector) override;
  void MergeInto(CameraCollector & collector) const override;

protected:
  void Save() override;
  void OrderCollectedData() override;

private:
  CameraProcessor m_processor;
};
}  // namespace routing_builder

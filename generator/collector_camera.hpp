#pragma once

#include "generator/collector_interface.hpp"

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

class CameraCollector : public generator::CollectorInterface
{
  friend class TestCameraCollector;

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

    uint64_t m_id = 0;
    double m_lon = 0.0;
    double m_lat = 0.0;
    uint32_t m_speedKmPH = 0;
    std::vector<uint64_t> m_ways;
  };

public:
  CameraCollector(std::string const & filename, IDRInterfacePtr cache);

  /// @name CollectorInterface overrides:
  /// @{
  std::shared_ptr<CollectorInterface> Clone(IDRInterfacePtr const & cache = {}) const override;
  void CollectFeature(feature::FeatureBuilder const & feature, OsmElement const & element) override;
  /// @}

  IMPLEMENT_COLLECTOR_IFACE(CameraCollector);
  void MergeInto(CameraCollector & collector) const;

protected:
  void Save() override;
  void OrderCollectedData() override;

  void FillCameraInWays();

  template <typename Fn>
  void ForEachCamera(Fn && toDo)
  {
    for (auto & p : m_speedCameras)
      toDo(p.second);
  }

private:
  IDRInterfacePtr m_cache;
  std::vector<uint64_t> m_roadOsmIDs;

  std::unordered_map<uint64_t, CameraInfo> m_speedCameras;
};
}  // namespace routing_builder

#pragma once

#include "generator/routing_helpers.hpp"

#include "routing/speed_camera_ser_des.hpp"

#include "indexer/data_source.hpp"

#include "coding/file_writer.hpp"

#include "geometry/point2d.hpp"

#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace generator
{
class CamerasInfoCollector
{
public:
  CamerasInfoCollector(std::string const & dataFilePath, std::string const & camerasInfoPath,
                       std::string const & osmIdsToFeatureIdsPath);

  void Serialize(FileWriter & writer) const;

private:
  struct Camera
  {
    inline static double constexpr kCoordEqualityEps = 1e-5;

    Camera(m2::PointD const & center, uint8_t maxSpeed, std::vector<routing::SpeedCameraMwmPosition> && ways)
      : m_data(center, maxSpeed, std::move(ways))
    {}

    void ParseDirection()
    {
      // After some research we understood, that camera's direction is a random option.
      // This fact was checked with randomly chosen cameras and google maps 3d view.
      // The real direction and the one indicated in the OSM differed.
      // So we left space in mwm for this feature and wait until good data appears in OSM
    }

    // Modify m_ways vector. Set for each way - id of the closest segment.
    void FindClosestSegment(FrozenDataSource const & dataSource, MwmSet::MwmId const & mwmId);

    /// \brief Try to FindClosestSegment in |m_ways|. Look to FindMyself() for more details.
    /// \return true if we could find closest segment. And false if we should use geometry index.
    bool FindClosestSegmentInInnerWays(FrozenDataSource const & dataSource, MwmSet::MwmId const & mwmId);

    /// \brief Use when |m_ways| is empty. Try to FindClosestSegment using geometry index.
    void FindClosestSegmentWithGeometryIndex(FrozenDataSource const & dataSource);

    // Returns empty object, if current feature - |wayId| is not the car road.
    // Otherwise returns id of segment from feature with id - |wayId|, which starts (or ends) at camera's
    // center and coefficient - where it placed at the segment: 0.0 (or 1.0).
    std::optional<std::pair<double, uint32_t>> FindMyself(uint32_t wayFeatureId, FrozenDataSource const & dataSource,
                                                          MwmSet::MwmId const & mwmId) const;

    void Serialize(FileWriter & writer, uint32_t & prevFeatureId) const;

    routing::SpeedCameraMetadata m_data;
  };

  inline static double constexpr kMaxDistFromCameraToClosestSegmentMeters = 20.0;
  inline static double constexpr kSearchCameraRadiusMeters = 10.0;

  bool ParseIntermediateInfo(std::string const & camerasInfoPath, routing::OsmIdToFeatureIds const & osmIdToFeatureIds);

  std::vector<Camera> m_cameras;
};

// To start building camera info, the following data must be ready:
// 1. GenerateIntermediateData(). Cached data about camera node to ways.
// 2. GenerateFeatures(). Data about cameras from OSM.
// 3. GenerateFinalFeatures(). Calculate some data for features.
// 4. BuildOffsetsTable().
// 5. BuildIndexFromDataFile() - for doing search in rect.
void BuildCamerasInfo(std::string const & dataFilePath, std::string const & camerasInfoPath,
                      std::string const & osmIdsToFeatureIdsPath);
}  // namespace generator

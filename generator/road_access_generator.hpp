#pragma once

#include "generator/intermediate_elements.hpp"
#include "generator/osm_element.hpp"

#include "routing/road_access.hpp"
#include "routing/vehicle_mask.hpp"

#include <cstdint>
#include <fstream>
#include <map>
#include <ostream>
#include <string>
#include <vector>

struct OsmElement;
class FeatureParams;

// The road accessibility information is collected in the same manner
// as the restrictions are.
// See generator/restriction_generator.hpp for details.
namespace routing
{
class RoadAccessTagProcessor
{
public:
  using TagMapping = map<OsmElement::Tag, RoadAccess::Type>;

  explicit RoadAccessTagProcessor(VehicleMask vehicleMask);

  void Process(OsmElement const & elem, std::ofstream & oss) const;

private:
  VehicleMask m_vehicleMask;
  TagMapping const * m_tagMapping;
};

class RoadAccessWriter
{
public:
  RoadAccessWriter();

  void Open(std::string const & filePath);

  void Process(OsmElement const & elem);

private:
  bool IsOpened() const;

  std::ofstream m_stream;
  std::vector<RoadAccessTagProcessor> m_tagProcessors;
};

class RoadAccessCollector
{
public:
  RoadAccessCollector(std::string const & dataFilePath, std::string const & roadAccessPath,
                      std::string const & osmIdsToFeatureIdsPath);

  std::map<VehicleMask, RoadAccess> const & GetRoadAccessByMask() const
  {
    return m_roadAccessByMask;
  }

  bool IsValid() const { return m_valid; }

private:
  std::map<VehicleMask, RoadAccess> m_roadAccessByMask;
  bool m_valid = true;
};

// The generator tool's interface to writing the section with
// road accessibility information for one mwm file.
void BuildRoadAccessInfo(std::string const & dataFilePath, std::string const & roadAccessPath,
                         std::string const & osmIdsToFeatureIdsPath);
}  // namespace routing

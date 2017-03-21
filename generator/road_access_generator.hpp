#pragma once

#include "generator/intermediate_elements.hpp"

#include "routing/road_access.hpp"

#include <cstdint>
#include <fstream>
#include <string>

struct OsmElement;
class FeatureParams;

// The road accessibility information is collected in the same manner
// as the restrictions are.
// See generator/restriction_generator.hpp for details.
namespace routing
{
class RoadAccessWriter
{
public:
  void Open(std::string const & filePath);

  void Process(OsmElement const & elem, FeatureParams const & params);

private:
  bool IsOpened() const;

  std::ofstream m_stream;
};

class RoadAccessCollector
{
public:
  RoadAccessCollector(std::string const & roadAccessPath, std::string const & osmIdsToFeatureIdsPath);

  RoadAccess const & GetRoadAccess() const { return m_roadAccess; }

  bool IsValid() const { return m_valid; }

private:
  RoadAccess m_roadAccess;
  bool m_valid = true;
};

// The generator tool's interface to writing the section with
// road accessibility information for one mwm file.
void BuildRoadAccessInfo(std::string const & dataFilePath, std::string const & roadAccessPath,
                         std::string const & osmIdsToFeatureIdsPath);
}  // namespace routing

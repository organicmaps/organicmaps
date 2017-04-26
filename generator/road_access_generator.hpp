#pragma once

#include "generator/intermediate_elements.hpp"
#include "generator/osm_element.hpp"

#include "routing/road_access.hpp"

#include <cstdint>
#include <fstream>
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

  RoadAccessTagProcessor(RouterType routerType);

  void Process(OsmElement const & elem, std::ofstream & oss) const;

private:
  RouterType m_routerType;
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

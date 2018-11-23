#pragma once

#include "generator/osm_element.hpp"

#include <string>
#include <vector>

namespace generator
{
/// \brief Collects all maxspeed tags value and saves them to a csv file.
/// Every line describes maxspeed, maxspeed:forward and maxspeed:backward
/// tags of features. The format of the lines is described below.
class MaxspeedsCollector
{
public:
  /// \param filePath path to csv file.
  explicit MaxspeedsCollector(std::string const & filePath) : m_filePath(filePath) {}
  ~MaxspeedsCollector() { Flush(); }

  void Process(OsmElement const & p);

private:
  void Flush();

  // |m_data| contains strings with maxspeed tags value for corresponding features in one of the
  // following formats
  // 1. osm id,units kmh or mph,maxspeed value
  // 2. osm id,units kmh or mph,maxspeed:forward value
  // 3. osm id,units kmh or mph,maxspeed:forward value,maxspeed:backward value
  // There are possible examples of strings contained in the list |m_data|:
  // 2343313,Metric,60
  // 13243214,Imperial,60
  // 3243245345,Metric,60,80
  // 134243,Imperial,30,50
  // 45432423,Metric,60,65534
  // 53445423,Metric,60,65533
  //
  // Note 1. 65534 means kNoneMaxSpeed and 65533 means kWalkMaxSpeed. They are constants for
  // maxspeed tag value "none" and "walk" correspondingly.
  // Note 2. Saying osm id means osm id of features with OsmElement::EntityType::Way type without
  // any prefixes. It's done so to simplify the debugging process. This way it's very easy knowing
  // osm id to see the feature on the web.
  // Note 3. A string is saved in |m_data| only if it's valid and parsed successfully
  // with ParseMaxspeedTag() function. That means all macro like RU:urban or GE:rural
  // are converted to an appropriate speed value and macro "none" and "walk" are converted
  // to |kNoneMaxSpeed| and |kWalkMaxSpeed|.
  std::vector<std::string> m_data;
  std::string m_filePath;
};
}  // namespace generator

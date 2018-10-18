#pragma once

#include "generator/osm_element.hpp"

#include <list>
#include <string>

namespace feature
{
/// \brief Saves csv file. Every line describe maxspeed, maxspeed:forward and maxspeed:backward
/// tags of linear features. The format of the lines is described below.
class MaxspeedBuilder
{
public:
  /// \pram filePath path to csv file.
  explicit MaxspeedBuilder(std::string const & filePath) : m_filePath(filePath) {}
  ~MaxspeedBuilder() { Flush(); }

  void operator()(OsmElement const & el);

private:
  void Flush();

  // |m_data| contains strings with maxspeed tags value for corresponding features in one of the
  // following formats
  // 1. osm id,maxspeed value
  // 2. osm id,maxspeed:forward value
  // 3. osm id,maxspeed:forward value,maxspeed:backward value
  // There's possble examples of strings contained in the list |m_data|:
  // 2343313,60
  // 3243245345,60,80
  // 32453452,RU:urban
  // 45432423,GE:urban,walk
  // 53445423,US:urban,40_mph
  //
  // Note. Saying osm id means osm id of features with OsmElement::EntityType::Way type without
  // any prefixes. It's done so to simplify the debugging process. This way it's very easy knowing
  // osm id to see the feature on the web.
  std::list<std::string> m_data;
  std::string m_filePath;
};
}  // namespace feature

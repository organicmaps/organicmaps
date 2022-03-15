#include "generator/generator_tests/common.hpp"

#include "platform/platform.hpp"

namespace generator_tests
{

OsmElement MakeOsmElement(uint64_t id, Tags const & tags, OsmElement::EntityType t)
{
  OsmElement el;
  el.m_id = id;
  el.m_type = t;
  for (auto const & t : tags)
    el.AddTag(t.first, t.second);

  return el;
}

std::string GetFileName(std::string const & filename)
{
  auto & platform = GetPlatform();
  return filename.empty() ? platform.TmpPathForFile() : platform.TmpPathForFile(filename);
}

}  // namespace generator_tests

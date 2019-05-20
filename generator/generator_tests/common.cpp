#include "generator/generator_tests/common.hpp"

#include "platform/platform.hpp"

#include "base/file_name_utils.hpp"

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

std::string GetFileName()
{
  auto & platform = GetPlatform();
  auto const tmpDir = platform.TmpDir();
  platform.SetWritableDirForTests(tmpDir);
  return platform.TmpPathForFile();
}
}  // namespace generator_tests

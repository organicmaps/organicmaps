#include "generator/generator_tests/common.hpp"

#include "generator/borders.hpp"

#include "indexer/classificator.hpp"

#include "platform/platform.hpp"

#include "base/file_name_utils.hpp"
#include "base/string_utils.hpp"

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

bool MakeFakeBordersFile(std::string const & intemediatePath, std::string const & filename)
{
  auto const borderPath = base::JoinPath(intemediatePath, BORDERS_DIR);
  auto & platform = GetPlatform();
  auto const code = platform.MkDir(borderPath);
  if (code != Platform::EError::ERR_OK && code != Platform::EError::ERR_FILE_ALREADY_EXISTS)
    return false;

  std::vector<m2::PointD> points = {{-180.0, -90.0}, {180.0, -90.0}, {180.0, 90.0}, {-180.0, 90.0},
                                    {-180.0, -90.0}};
  borders::DumpBorderToPolyFile(borderPath, filename, {m2::RegionD{points}});
  return true;
}
}  // namespace generator_tests

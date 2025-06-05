#include "poly_borders/poly_borders_tests/tools.hpp"

#include "poly_borders/borders_data.hpp"

#include "generator/borders.hpp"

#include "geometry/region2d.hpp"

#include "base/file_name_utils.hpp"

#include <string>
#include <vector>

using namespace platform::tests_support;

namespace
{
std::vector<m2::RegionD> ConvertFromPointsVector(std::vector<std::vector<m2::PointD>> const & polygons)
{
  std::vector<m2::RegionD> res;
  res.reserve(polygons.size());
  for (auto const & polygon : polygons)
    res.emplace_back(polygon);

  return res;
}
}  // namespace

namespace poly_borders
{
std::shared_ptr<ScopedFile> CreatePolyBorderFileByPolygon(std::string const & relativeDirPath, std::string const & name,
                                                          std::vector<std::vector<m2::PointD>> const & polygons)
{
  std::string path = base::JoinPath(relativeDirPath, name + BordersData::kBorderExtension);

  auto file = std::make_shared<ScopedFile>(path, ScopedFile::Mode::Create);

  auto const targetDir = base::GetDirectory(file->GetFullPath());

  auto const regions = ConvertFromPointsVector(polygons);
  borders::DumpBorderToPolyFile(targetDir, name, regions);

  return file;
}
}  // namespace poly_borders

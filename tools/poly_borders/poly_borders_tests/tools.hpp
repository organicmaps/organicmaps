#pragma once

#include "platform/platform_tests_support/scoped_file.hpp"

#include "geometry/point2d.hpp"

#include <memory>
#include <string>
#include <vector>

namespace poly_borders
{
std::shared_ptr<platform::tests_support::ScopedFile> CreatePolyBorderFileByPolygon(
    std::string const & relativeDirPath, std::string const & name,
    std::vector<std::vector<m2::PointD>> const & polygons);
}  // namespace poly_borders

#pragma once
#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"

#include <functional>
#include <string>

class DataSource;

namespace routing
{
using CountryFileGetterFn = std::function<std::string(m2::PointD const &)>;
using CountryRectFn = std::function<m2::RectD(std::string const & countryId)>;

class Checkpoints;
class NumMwmIds;
}  // namespace routing

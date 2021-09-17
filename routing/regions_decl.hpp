#pragma once
#include "geometry/point2d.hpp"

#include <functional>
#include <memory>
#include <string>

class DataSource;

namespace routing
{
using CountryFileGetterFn = std::function<std::string(m2::PointD const &)>;

class Checkpoints;
class NumMwmIds;
} // namespace routing

#pragma once

namespace routing
{
namespace transit
{
double constexpr kTransitMaxSpeedKMpH = 400.0;
// @TODO(bykoianko, Zverik) Edge and gate weights should be valid always valid. This weights should come
// from transit graph json. But now it'not so. |kTransitAverageSpeedKMpH| should be used now only for
// weight calculating weight at transit section generation stage and should be removed later.
double constexpr kTransitAverageSpeedKMpH = 40.0;
}  // namespace transit
}  // namespace routing

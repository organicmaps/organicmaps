#pragma once

namespace routing
{
namespace transit
{
double constexpr kTransitMaxSpeedKMpH = 400.0;
// @TODO(bykoianko, Zverik) Edge and gate weights should be always valid. This weights should come
// from transit graph json. But now it's not so. |kTransitAverageSpeedMPS| should be used now only for
// weight calculating at transit section generation stage and the constant should be removed later.
double constexpr kTransitAverageSpeedMPS = 11.0;
}  // namespace transit
}  // namespace routing

#include "sailfish/elevation.hpp"

#include "map/elevation_info.hpp"

#include "platform/distance.hpp"

#include <QVariantList>

namespace sailfish
{
QVariantMap ElevationChart(ElevationInfo const & elevation, geometry::Altitude minAltitude,
                           geometry::Altitude maxAltitude)
{
  // Thinned to what a phone wide chart shows; the last point is kept, so that the profile ends at length.
  size_t constexpr kMaxPoints = 600;
  QVariantList profile;
  double length = 0;
  size_t const count = elevation.GetSize();
  size_t const step = count / kMaxPoints + 1;
  size_t i = 0;
  elevation.ForEachPoint([&](double distance, geometry::Altitude altitude)
  {
    if (i % step == 0 || i + 1 == count)
      profile << distance << altitude;
    ++i;
    length = distance;
  });
  return {{"profile", profile},
          {"length", length},
          {"min", QString::fromStdString(platform::Distance::FormatAltitude(minAltitude))},
          {"max", QString::fromStdString(platform::Distance::FormatAltitude(maxAltitude))}};
}
}  // namespace sailfish

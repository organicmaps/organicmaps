#pragma once

#include "geometry/point_with_altitude.hpp"

#include <QVariantMap>

class ElevationInfo;

namespace sailfish
{
// What ElevationChart.qml shows: {profile, length, min, max}. The profile has distance and altitude pairs in
// meters in one flat list, length is its last distance, min and max the formatted altitude range.
QVariantMap ElevationChart(ElevationInfo const & elevation, geometry::Altitude minAltitude,
                           geometry::Altitude maxAltitude);
}  // namespace sailfish

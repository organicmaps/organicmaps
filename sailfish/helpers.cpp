#include "sailfish/helpers.hpp"

namespace sailfish
{
MapStyle BaseMapStyle(bool dark, bool outdoors)
{
  if (outdoors)
    return dark ? MapStyleOutdoorsDark : MapStyleOutdoorsLight;
  return dark ? MapStyleDefaultDark : MapStyleDefaultLight;
}
}  // namespace sailfish

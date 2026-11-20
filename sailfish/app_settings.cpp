#include "sailfish/app_settings.hpp"

#include "map/framework.hpp"

namespace sailfish
{
AppSettings::AppSettings(Framework & framework, QObject * parent) : QObject(parent), m_framework(framework) {}

void AppSettings::applyMapAppearance(bool dark)
{
  MapStyle const current = m_framework.GetMapStyle();
  MapStyle const style = dark ? GetDarkMapStyleVariant(current) : GetLightMapStyleVariant(current);
  if (style != current)
    m_framework.SetMapStyle(style);
}
}  // namespace sailfish

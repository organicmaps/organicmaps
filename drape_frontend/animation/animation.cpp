#include "animation.hpp"

namespace df
{

bool Animation::GetCachedProperty(TPropertyCache const & properties, Object object, ObjectProperty property, PropertyValue & value)
{
  auto const it = properties.find(make_pair(object, property));
  if (it != properties.end())
  {
    value = it->second;
    return true;
  }
  return false;
}

void Animation::GetCurrentScreen(TPropertyCache const & properties, ScreenBase const & screen, ScreenBase & currentScreen)
{
  currentScreen = screen;

  if (!properties.empty())
  {
    double scale = currentScreen.GetScale();
    double angle = currentScreen.GetAngle();
    m2::PointD pos = currentScreen.GlobalRect().GlobalZero();

    PropertyValue value;
    if (GetCachedProperty(properties, Object::MapPlane, ObjectProperty::Scale, value))
      scale = value.m_valueD;

    if (GetCachedProperty(properties, Object::MapPlane, ObjectProperty::Angle, value))
      angle = value.m_valueD;

    if (GetCachedProperty(properties, Object::MapPlane, ObjectProperty::Position, value))
      pos = value.m_valuePointD;

    currentScreen.SetFromParams(pos, angle, scale);
  }
}

bool Animation::CouldBeBlendedWith(Animation const & animation) const
{
  bool hasSameObject = false;
  TAnimObjects const & objects = animation.GetObjects();
  for (auto const & object : objects)
  {
    if (HasObject(object))
    {
      hasSameObject = true;
      break;
    }
  }

  return !hasSameObject || ((GetType() != animation.GetType()) &&
      m_couldBeBlended && animation.m_couldBeBlended);
}

bool Animation::HasTargetProperty(Object object, ObjectProperty property) const
{
  return HasProperty(object, property);
}

} // namespace df

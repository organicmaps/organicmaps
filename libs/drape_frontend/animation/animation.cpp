#include "drape_frontend/animation/animation.hpp"

#include <algorithm>

namespace df
{

// static
bool Animation::GetCachedProperty(TPropertyCache const & properties, Object object, ObjectProperty property,
                                  PropertyValue & value)
{
  auto const it = properties.find(std::make_pair(object, property));
  if (it != properties.end())
  {
    value = it->second;
    return true;
  }
  return false;
}

// static
void Animation::GetCurrentScreen(TPropertyCache const & properties, ScreenBase const & screen,
                                 ScreenBase & currentScreen)
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

bool Animation::HasSameObjects(Animation const & animation) const
{
  TAnimObjects const & objects = animation.GetObjects();
  for (auto const & object : objects)
    if (HasObject(object))
      return true;
  return false;
}

bool Animation::CouldBeBlendedWith(Animation const & animation) const
{
  return !HasSameObjects(animation) ||
         ((GetType() != animation.GetType()) && m_couldBeBlended && animation.m_couldBeBlended);
}

bool Animation::HasTargetProperty(Object object, ObjectProperty property) const
{
  return HasProperty(object, property);
}

// static
bool Animation::GetMinDuration(Interpolator const & interpolator, double & minDuration)
{
  if (interpolator.IsActive())
  {
    double const duration = interpolator.GetMinDuration();
    if (duration >= 0.0)
      minDuration = minDuration >= 0.0 ? std::min(duration, minDuration) : duration;
    else
      return false;
  }
  return true;
}

// static
bool Animation::GetMaxDuration(Interpolator const & interpolator, double & maxDuration)
{
  if (interpolator.IsActive())
  {
    double const duration = interpolator.GetMaxDuration();
    if (duration >= 0.0)
      maxDuration = maxDuration >= 0.0 ? std::max(duration, maxDuration) : duration;
    else
      return false;
  }
  return true;
}

std::string DebugPrint(Animation::Type const & type)
{
  switch (type)
  {
  case Animation::Type::Sequence: return "Sequence";
  case Animation::Type::Parallel: return "Parallel";
  case Animation::Type::MapLinear: return "MapLinear";
  case Animation::Type::MapScale: return "MapScale";
  case Animation::Type::MapFollow: return "MapFollow";
  case Animation::Type::Arrow: return "Arrow";
  case Animation::Type::KineticScroll: return "KineticScroll";
  }
  return "Unknown type";
}

std::string DebugPrint(Animation::Object const & object)
{
  switch (object)
  {
  case Animation::Object::MyPositionArrow: return "MyPositionArrow";
  case Animation::Object::MapPlane: return "MapPlane";
  case Animation::Object::Selection: return "Selection";
  }
  return "Unknown object";
}

std::string DebugPrint(Animation::ObjectProperty const & property)
{
  switch (property)
  {
  case Animation::ObjectProperty::Position: return "Position";
  case Animation::ObjectProperty::Scale: return "Scale";
  case Animation::ObjectProperty::Angle: return "Angle";
  }
  return "Unknown property";
}

}  // namespace df

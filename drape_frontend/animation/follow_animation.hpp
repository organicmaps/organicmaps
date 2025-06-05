#pragma once

#include "animation.hpp"
#include "interpolators.hpp"

namespace df
{

class MapFollowAnimation : public Animation
{
public:
  MapFollowAnimation(ScreenBase const & screen, m2::PointD const & globalUserPosition,
                     m2::PointD const & endPixelPosition, double endScale, double endAngle, bool isAutoZoom);

  void Init(ScreenBase const & screen, TPropertyCache const & properties) override;

  Animation::Type GetType() const override { return Animation::Type::MapFollow; }

  TAnimObjects const & GetObjects() const override { return m_objects; }

  bool HasObject(Object object) const override { return object == Animation::Object::MapPlane; }

  TObjectProperties const & GetProperties(Object object) const override;
  bool HasProperty(Object object, ObjectProperty property) const override;

  void Advance(double elapsedSeconds) override;
  void Finish() override;

  void SetMaxDuration(double maxDuration) override;
  void SetMinDuration(double minDuration) override;
  double GetDuration() const override;
  double GetMinDuration() const override;
  double GetMaxDuration() const override;
  bool IsFinished() const override;

  bool GetProperty(Object object, ObjectProperty property, PropertyValue & value) const override;
  bool GetTargetProperty(Object object, ObjectProperty property, PropertyValue & value) const override;

  bool IsAutoZoom() const { return m_isAutoZoom; }

  bool HasScale() const;
  bool HasPixelOffset() const;

private:
  bool GetProperty(Object object, ObjectProperty property, bool targetValue, PropertyValue & value) const;
  double CalculateDuration() const;

  bool m_isAutoZoom;
  ScaleInterpolator m_scaleInterpolator;
  AngleInterpolator m_angleInterpolator;
  PositionInterpolator m_offsetInterpolator;

  m2::PointD const m_globalPosition;
  m2::PointD const m_endPixelPosition;
  double const m_endScale;
  double const m_endAngle;

  m2::PointD m_offset;

  TObjectProperties m_properties;
  TAnimObjects m_objects;
};

}  // namespace df

#pragma once

#include "animation.hpp"
#include "interpolators.hpp"

namespace df
{

class MapFollowAnimation : public Animation
{
public:
  MapFollowAnimation(ScreenBase const & screen,
                     m2::PointD const & globalUserPosition,
                     m2::PointD const & endPixelPosition,
                     double startScale, double endScale,
                     double startAngle, double endAngle);

  Animation::Type GetType() const override { return Animation::MapFollow; }

  TAnimObjects const & GetObjects() const override
  {
    return m_objects;
  }

  bool HasObject(TObject object) const override
  {
    return object == Animation::MapPlane;
  }

  TObjectProperties const & GetProperties(TObject object) const override;
  bool HasProperty(TObject object, TProperty property) const override;

  void Advance(double elapsedSeconds) override;
  void Finish() override;

  void SetMaxDuration(double maxDuration) override;
  double GetDuration() const override;
  bool IsFinished() const override;

  bool GetProperty(TObject object, TProperty property, PropertyValue & value) const override;
  bool GetTargetProperty(TObject object, TProperty property, PropertyValue & value) const override;

  bool HasScale() const;
  bool HasPixelOffset() const;

private:
  bool GetProperty(TObject object, TProperty property, bool targetValue, PropertyValue & value) const;
  double CalculateDuration() const;

  ScaleInterpolator m_scaleInterpolator;
  AngleInterpolator m_angleInterpolator;
  PositionInterpolator m_offsetInterpolator;

  m2::PointD const m_globalPosition;
  m2::PointD const m_endPixelPosition;

  m2::PointD m_offset;

  TObjectProperties m_properties;
  TAnimObjects m_objects;
};

} // namespace df

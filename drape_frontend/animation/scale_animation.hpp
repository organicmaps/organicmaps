#pragma once

#include "animation.hpp"
#include "interpolators.hpp"

namespace df
{

class MapScaleAnimation : public Animation
{
public:
  MapScaleAnimation(double startScale, double endScale, m2::PointD const & globalScaleCenter,
                    m2::PointD const & pxScaleCenter);

  void Init(ScreenBase const & screen, TPropertyCache const & properties) override;

  Animation::Type GetType() const override { return Animation::Type::MapScale; }

  TAnimObjects const & GetObjects() const override { return m_objects; }

  bool HasObject(Object object) const override { return object == Animation::Object::MapPlane; }

  TObjectProperties const & GetProperties(Object object) const override;
  bool HasProperty(Object object, ObjectProperty property) const override;

  void Advance(double elapsedSeconds) override;
  void Finish() override;

  void SetMaxDuration(double maxDuration) override;
  void SetMinDuration(double minDuration) override;
  double GetDuration() const override;
  double GetMaxDuration() const override;
  double GetMinDuration() const override;
  bool IsFinished() const override;

  bool GetProperty(Object object, ObjectProperty property, PropertyValue & value) const override;
  bool GetTargetProperty(Object object, ObjectProperty property, PropertyValue & value) const override;

private:
  bool GetProperty(Object object, ObjectProperty property, bool targetValue, PropertyValue & value) const;

  ScaleInterpolator m_scaleInterpolator;
  m2::PointD const m_pxScaleCenter;
  m2::PointD const m_globalScaleCenter;
  TObjectProperties m_properties;
  TAnimObjects m_objects;
};

}  // namespace df

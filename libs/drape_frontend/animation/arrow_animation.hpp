#pragma once

#include "animation.hpp"
#include "interpolators.hpp"

namespace df
{

class ArrowAnimation : public Animation
{
public:
  ArrowAnimation(m2::PointD const & startPos, m2::PointD const & endPos, double moveDuration, double startAngle,
                 double endAngle);

  void Init(ScreenBase const & screen, TPropertyCache const & properties) override;

  Animation::Type GetType() const override { return Animation::Type::Arrow; }

  TAnimObjects const & GetObjects() const override;
  bool HasObject(Object object) const override;
  TObjectProperties const & GetProperties(Object object) const override;
  bool HasProperty(Object object, ObjectProperty property) const override;

  void SetMaxDuration(double maxDuration) override;
  void SetMinDuration(double minDuration) override;
  double GetDuration() const override;
  double GetMinDuration() const override;
  double GetMaxDuration() const override;
  bool IsFinished() const override;

  void Advance(double elapsedSeconds) override;
  void Finish() override;

  bool GetProperty(Object object, ObjectProperty property, PropertyValue & value) const override;
  bool GetTargetProperty(Object object, ObjectProperty property, PropertyValue & value) const override;

private:
  bool GetProperty(Object object, ObjectProperty property, bool targetValue, PropertyValue & value) const;

  TAnimObjects m_objects;
  TObjectProperties m_properties;
  PositionInterpolator m_positionInterpolator;
  AngleInterpolator m_angleInterpolator;
};

}  // namespace df

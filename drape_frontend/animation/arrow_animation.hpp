#pragma once

#include "animation.hpp"
#include "interpolators.hpp"

namespace df
{

class ArrowAnimation : public Animation
{
public:
  ArrowAnimation(m2::PointD const & startPos, m2::PointD const & endPos, double moveDuration,
                 double startAngle, double endAngle);

  Animation::Type GetType() const override { return Animation::Arrow; }

  TAnimObjects const & GetObjects() const override;
  bool HasObject(TObject object) const override;
  TObjectProperties const & GetProperties(TObject object) const override;
  bool HasProperty(TObject object, TProperty property) const override;

  void SetMaxDuration(double maxDuration) override;
  double GetDuration() const override;
  bool IsFinished() const override;

  void Advance(double elapsedSeconds) override;
  void Finish() override;

  bool GetProperty(TObject object, TProperty property, PropertyValue & value) const override;
  bool GetTargetProperty(TObject object, TProperty property, PropertyValue & value) const override;

private:
  bool GetProperty(TObject object, TProperty property, bool targetValue, PropertyValue & value) const;

  TAnimObjects m_objects;
  TObjectProperties m_properties;
  PositionInterpolator m_positionInterpolator;
  AngleInterpolator m_angleInterpolator;
};

} // namespace df

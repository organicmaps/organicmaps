#pragma once

#include "animation.hpp"
#include "interpolators.hpp"

namespace df
{

class MapLinearAnimation : public Animation
{
public:
  MapLinearAnimation(m2::PointD const & startPos, m2::PointD const & endPos,
                     double startAngle, double endAngle,
                     double startScale, double endScale, ScreenBase const & convertor);
  MapLinearAnimation();

  void SetMove(m2::PointD const & startPos, m2::PointD const & endPos, ScreenBase const & convertor);
  void SetRotate(double startAngle, double endAngle);
  void SetScale(double startScale, double endScale);

  Animation::Type GetType() const override { return Animation::MapLinear; }

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

  void SetMaxScaleDuration(double maxDuration);

private:
  bool GetProperty(TObject object, TProperty property, bool targetValue, PropertyValue & value) const;

  AngleInterpolator m_angleInterpolator;
  PositionInterpolator m_positionInterpolator;
  ScaleInterpolator m_scaleInterpolator;
  TObjectProperties m_properties;
  TAnimObjects m_objects;
};

} // namespace df

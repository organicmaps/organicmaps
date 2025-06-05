#pragma once

#include "animation.hpp"
#include "interpolators.hpp"

namespace df
{
class MapLinearAnimation : public Animation
{
public:
  MapLinearAnimation(m2::PointD const & startPos, m2::PointD const & endPos, double startAngle, double endAngle,
                     double startScale, double endScale, ScreenBase const & convertor);
  MapLinearAnimation();

  void Init(ScreenBase const & screen, TPropertyCache const & properties) override;

  void SetMove(m2::PointD const & startPos, m2::PointD const & endPos, ScreenBase const & convertor);
  void SetMove(m2::PointD const & startPos, m2::PointD const & endPos, m2::RectD const & viewportRect, double scale);
  void SetRotate(double startAngle, double endAngle);
  void SetScale(double startScale, double endScale);

  Animation::Type GetType() const override { return Animation::Type::MapLinear; }

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

  void SetMaxScaleDuration(double maxDuration);

private:
  bool GetProperty(Object object, ObjectProperty property, bool targetValue, PropertyValue & value) const;

  AngleInterpolator m_angleInterpolator;
  PositionInterpolator m_positionInterpolator;
  ScaleInterpolator m_scaleInterpolator;
  TObjectProperties m_properties;
  TAnimObjects m_objects;
};
}  // namespace df

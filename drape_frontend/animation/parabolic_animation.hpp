#pragma once

#include "animation.hpp"

namespace df
{

// Default duration for parabolic animations
double constexpr kParabolicAnimationDuration = 1.0;

class ParabolicAnimation : public Animation
{
public:
  ParabolicAnimation(ScreenBase const & startScreen,
                     m2::PointD const & startPos, m2::PointD const & endPos,
                     double startScale, double endScale,
                     double startAngle, double endAngle,
                     double peakScale);

  void Init(ScreenBase const & screen, TPropertyCache const & properties) override;

  Animation::Type GetType() const override { return Animation::Type::MapParabolic; }

  TAnimObjects const & GetObjects() const override
  {
     return m_objects;
  }

  bool HasObject(Object object) const override
  {
    return m_objects.find(object) != m_objects.end();
  }

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
  bool GetProperty(Object object, ObjectProperty property, bool targetValue, PropertyValue & value) const;

private:
  double GetT() const;
  double GetCurrentScale() const;
  m2::PointD GetCurrentPosition() const;
  double GetCurrentAngle() const;

  m2::PointD m_startPos;
  m2::PointD m_endPos;
  double m_startScale;
  double m_endScale;
  double m_peakScale;
  double m_startAngle;
  double m_endAngle;
  double m_duration;
  double m_elapsedTime;

  TAnimObjects m_objects;
  TObjectProperties m_properties;
};

} // namespace df 

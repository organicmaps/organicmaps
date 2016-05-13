#pragma once

#include "animation.hpp"
#include "interpolators.hpp"

namespace df
{

class PerspectiveSwitchAnimation : public Animation
{
public:
  PerspectiveSwitchAnimation(double startAngle, double endAngle, double angleFOV);

  static double GetRotateDuration(double startAngle, double endAngle);

  Animation::Type GetType() const override { return Animation::MapPerspective; }

  TAnimObjects const & GetObjects() const override
  {
     return m_objects;
  }

  bool HasObject(TObject object) const override
  {
    return m_objects.find(object) != m_objects.end();
  }

  TObjectProperties const & GetProperties(TObject object) const override;
  bool HasProperty(TObject object, TProperty property) const override;

  void Advance(double elapsedSeconds) override;
  void Finish() override;

  void OnStart() override;
  void OnFinish() override;

  void SetMaxDuration(double maxDuration) override;
  double GetDuration() const override;
  bool IsFinished() const override;

  bool GetProperty(TObject object, TProperty property, PropertyValue & value) const override;

private:
  AngleInterpolator m_angleInterpolator;
  double m_startAngle;
  double m_endAngle;
  double m_angleFOV;

  bool m_isEnablePerspectiveAnim;
  mutable bool m_needPerspectiveSwitch;
  TAnimObjects m_objects;
  TObjectProperties m_properties;
};

} // namespace df


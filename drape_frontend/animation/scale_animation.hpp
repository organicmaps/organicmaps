#pragma once

#include "animation.hpp"
#include "interpolators.hpp"

namespace df
{

class MapScaleAnimation : public Animation
{
public:
  MapScaleAnimation(double startScale, double endScale,
                    m2::PointD const & globalScaleCenter, m2::PointD const & pixelCenterOffset);

  Animation::Type GetType() const override { return Animation::MapScale; }

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

private:
  bool GetProperty(TObject object, TProperty property, bool targetValue, PropertyValue & value) const;

  ScaleInterpolator m_scaleInterpolator;
  m2::PointD const m_pixelCenterOffset;
  m2::PointD const m_globalScaleCenter;
  TObjectProperties m_properties;
  TAnimObjects m_objects;
};

} // namespace df


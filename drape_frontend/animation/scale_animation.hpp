#pragma once

#include "animation.hpp"
#include "interpolators.hpp"

namespace df
{

class MapScaleAnimation : public Animation
{
public:
  MapScaleAnimation(double startScale, double endScale,
                    m2::PointD const & globalPosition, m2::PointD const & offset);

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

private:
  ScaleInterpolator m_scaleInterpolator;
  m2::PointD const m_pixelOffset;
  m2::PointD const m_globalPosition;
  TObjectProperties m_properties;
  TAnimObjects m_objects;
};

} // namespace df


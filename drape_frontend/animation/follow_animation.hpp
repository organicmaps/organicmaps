#pragma once

#include "animation.hpp"
#include "interpolators.hpp"

namespace df
{

class MapFollowAnimation : public Animation
{
public:
  MapFollowAnimation(m2::PointD const & globalPosition,
                     double startScale, double endScale,
                     double startAngle, double endAngle,
                     m2::PointD const & startPixelPosition,
                     m2::PointD const & endPixelPosition,
                     m2::RectD const & pixelRect);

  static m2::PointD CalculateCenter(ScreenBase const & screen, m2::PointD const & userPos,
                                    m2::PointD const & pixelPos, double azimuth);

  static m2::PointD CalculateCenter(double scale, m2::RectD const & pixelRect,
                                    m2::PointD const & userPos, m2::PointD const & pixelPos, double azimuth);

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

  bool HasScale() const;
  bool HasPixelOffset() const;

private:
  double CalculateDuration() const;

  ScaleInterpolator m_scaleInterpolator;
  PositionInterpolator m_pixelPosInterpolator;
  AngleInterpolator m_angleInterpolator;

  m2::PointD const m_globalPosition;

  TObjectProperties m_properties;
  TAnimObjects m_objects;
};

} // namespace df

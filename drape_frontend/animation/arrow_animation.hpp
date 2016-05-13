#pragma once

#include "animation.hpp"
#include "interpolators.hpp"

namespace df
{

//TODO (in future): implement arrow animation on new animation system.
/*class ArrowAnimation : public Animation
{
public:
  ArrowAnimation(m2::PointD const & startPos, m2::PointD const & endPos,
                 double startAngle, double endAngle, ScreenBase const & convertor)
    : Animation(false, false)
  {
    m_positionInterpolator.reset(new PositionInterpolator(startPos, endPos, convertor));
    m_angleInterpolator.reset(new AngleInterpolator(startAngle, endAngle));
    m_objects.insert(Animation::MyPositionArrow);
    m_properties.insert(Animation::Position);
    m_properties.insert(Animation::Angle);
  }

  Animation::Type GetType() const override { return Animation::Arrow; }

  TAnimObjects const & GetObjects() const override
  {
     return m_objects;
  }

  bool HasObject(TObject object) const override
  {
    return object == Animation::MyPositionArrow;
  }

  TObjectProperties const & GetProperties(TObject object) const override;
  bool HasProperty(TObject object, TProperty property) const override;

  void Advance(double elapsedSeconds) override;
  void Finish() override;

private:
  drape_ptr<PositionInterpolator> m_positionInterpolator;
  drape_ptr<AngleInterpolator> m_angleInterpolator;
  TAnimObjects m_objects;
  TObjectProperties m_properties;
};*/

} // namespace df

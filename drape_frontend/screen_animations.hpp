#pragma once

#include "screen_operations.hpp"

#include "animation/animation.hpp"

namespace df
{

extern string const kPrettyMoveAnim;
extern string const kPrettyFollowAnim;

class SequenceAnimation;
class MapLinearAnimation;
class MapFollowAnimation;
class MapScaleAnimation;

drape_ptr<SequenceAnimation> GetPrettyMoveAnimation(ScreenBase const & screen,
                                                    m2::AnyRectD const & startRect, m2::AnyRectD const & endRect);
drape_ptr<SequenceAnimation> GetPrettyMoveAnimation(ScreenBase const & screen, double startScale, double endScale,
                                                    m2::PointD const & startPt, m2::PointD const & endPt);

drape_ptr<SequenceAnimation> GetPrettyFollowAnimation(ScreenBase const & screen, double startScale, double endScale,
                                                      m2::PointD const & startPt, m2::PointD const & userPos,
                                                      double targetAngle, m2::PointD const & endPixelPos);

drape_ptr<MapLinearAnimation> GetSetRectAnimation(ScreenBase const & screen,
                                                  m2::AnyRectD const & startRect, m2::AnyRectD const & endRect);

drape_ptr<MapFollowAnimation> GetFollowAnimation(ScreenBase const & startScreen, m2::PointD const & userPos,
                                                 double targetScale, double targetAngle, m2::PointD const & endPixelPos);

drape_ptr<MapScaleAnimation> GetScaleAnimation(ScreenBase const & startScreen, m2::PointD pxScaleCenter,
                                               m2::PointD glbScaleCenter, double factor);

} // namespace df


#pragma once

#include "drape_frontend/animation/animation.hpp"
#include "drape_frontend/screen_operations.hpp"

#include <string>

namespace df
{

extern std::string const kPrettyMoveAnim;
extern std::string const kPrettyFollowAnim;
extern std::string const kParallelFollowAnim;
extern std::string const kParallelLinearAnim;

class SequenceAnimation;
class MapLinearAnimation;
class MapFollowAnimation;
class MapScaleAnimation;

drape_ptr<SequenceAnimation> GetPrettyMoveAnimation(ScreenBase const & startScreen, ScreenBase const & endScreen);
drape_ptr<SequenceAnimation> GetPrettyMoveAnimation(ScreenBase const & screen, m2::AnyRectD const & startRect,
                                                    m2::AnyRectD const & endRect);
drape_ptr<SequenceAnimation> GetPrettyMoveAnimation(ScreenBase const & screen, double startScale, double endScale,
                                                    m2::PointD const & startPt, m2::PointD const & endPt);

drape_ptr<SequenceAnimation> GetPrettyFollowAnimation(ScreenBase const & startScreen, m2::PointD const & userPos,
                                                      double targetScale, double targetAngle,
                                                      m2::PointD const & endPixelPos);

drape_ptr<MapLinearAnimation> GetRectAnimation(ScreenBase const & startScreen, ScreenBase const & endScreen);
drape_ptr<MapLinearAnimation> GetSetRectAnimation(ScreenBase const & screen, m2::AnyRectD const & startRect,
                                                  m2::AnyRectD const & endRect);

drape_ptr<MapFollowAnimation> GetFollowAnimation(ScreenBase const & startScreen, m2::PointD const & userPos,
                                                 double targetScale, double targetAngle, m2::PointD const & endPixelPos,
                                                 bool isAutoZoom);

drape_ptr<MapScaleAnimation> GetScaleAnimation(ScreenBase const & startScreen, m2::PointD pxScaleCenter,
                                               m2::PointD glbScaleCenter, double factor);

}  // namespace df

#include "animation_constants.hpp"
#include "screen_animations.hpp"
#include "screen_operations.hpp"

#include "animation/follow_animation.hpp"
#include "animation/linear_animation.hpp"
#include "animation/scale_animation.hpp"
#include "animation/sequence_animation.hpp"

namespace df
{

string const kPrettyMoveAnim = "PrettyMove";
string const kPrettyFollowAnim = "PrettyFollow";

drape_ptr<SequenceAnimation> GetPrettyMoveAnimation(ScreenBase const & screen,
                                                    m2::AnyRectD const & startRect, m2::AnyRectD const & endRect)
{
  double const startScale = CalculateScale(screen.PixelRect(), startRect.GetLocalRect());
  double const endScale = CalculateScale(screen.PixelRect(), endRect.GetLocalRect());

  return GetPrettyMoveAnimation(screen, startScale, endScale, startRect.GlobalCenter(), endRect.GlobalCenter());
}

drape_ptr<SequenceAnimation> GetPrettyMoveAnimation(ScreenBase const & screen, double startScale, double endScale,
                                                    m2::PointD const & startPt, m2::PointD const & endPt)
{
  double const moveDuration = PositionInterpolator::GetMoveDuration(startPt, endPt, screen);
  double const scaleFactor = moveDuration / kMaxAnimationTimeSec * 2.0;

  auto sequenceAnim = make_unique_dp<SequenceAnimation>();
  sequenceAnim->SetCustomType(kPrettyMoveAnim);

  auto zoomOutAnim = make_unique_dp<MapLinearAnimation>();
  zoomOutAnim->SetScale(startScale, startScale * scaleFactor);
  zoomOutAnim->SetMaxDuration(kMaxAnimationTimeSec * 0.5);

  //TODO (in future): Pass fixed duration instead of screen.
  auto moveAnim = make_unique_dp<MapLinearAnimation>();
  moveAnim->SetMove(startPt, endPt, screen);
  moveAnim->SetMaxDuration(kMaxAnimationTimeSec);

  auto zoomInAnim = make_unique_dp<MapLinearAnimation>();
  zoomInAnim->SetScale(startScale * scaleFactor, endScale);
  zoomInAnim->SetMaxDuration(kMaxAnimationTimeSec * 0.5);

  sequenceAnim->AddAnimation(move(zoomOutAnim));
  sequenceAnim->AddAnimation(move(moveAnim));
  sequenceAnim->AddAnimation(move(zoomInAnim));
  return sequenceAnim;
}

drape_ptr<SequenceAnimation> GetPrettyFollowAnimation(ScreenBase const & screen, double startScale, double endScale,
                                                      m2::PointD const & startPt, m2::PointD const & userPos,
                                                      double targetAngle, m2::PointD const & endPixelPos)
{
  double const moveDuration = PositionInterpolator::GetMoveDuration(startPt, userPos, screen);
  double const scaleFactor = moveDuration / kMaxAnimationTimeSec * 2.0;

  auto sequenceAnim = make_unique_dp<SequenceAnimation>();
  sequenceAnim->SetCustomType(kPrettyFollowAnim);

  auto zoomOutAnim = make_unique_dp<MapLinearAnimation>();
  zoomOutAnim->SetScale(startScale, startScale * scaleFactor);
  zoomOutAnim->SetMaxDuration(kMaxAnimationTimeSec * 0.5);

  //TODO (in future): Pass fixed duration instead of screen.
  auto moveAnim = make_unique_dp<MapLinearAnimation>();
  moveAnim->SetMove(startPt, userPos, screen);
  moveAnim->SetMaxDuration(kMaxAnimationTimeSec);

  auto followAnim = make_unique_dp<MapFollowAnimation>(userPos, startScale * scaleFactor, endScale,
                                                       screen.GetAngle(), targetAngle,
                                                       screen.PixelRect().Center(), endPixelPos, screen.PixelRect());
  followAnim->SetMaxDuration(kMaxAnimationTimeSec * 0.5);

  sequenceAnim->AddAnimation(move(zoomOutAnim));
  sequenceAnim->AddAnimation(move(moveAnim));
  sequenceAnim->AddAnimation(move(followAnim));
  return sequenceAnim;
}

drape_ptr<MapLinearAnimation> GetSetRectAnimation(ScreenBase const & screen,
                                                  m2::AnyRectD const & startRect, m2::AnyRectD const & endRect)
{
  auto anim = make_unique_dp<MapLinearAnimation>();

  double const startScale = CalculateScale(screen.PixelRect(), startRect.GetLocalRect());
  double const endScale = CalculateScale(screen.PixelRect(), endRect.GetLocalRect());

  anim->SetRotate(startRect.Angle().val(), endRect.Angle().val());
  anim->SetMove(startRect.GlobalCenter(), endRect.GlobalCenter(), screen);
  anim->SetScale(startScale, endScale);
  anim->SetMaxScaleDuration(kMaxAnimationTimeSec);

  return anim;
}

drape_ptr<MapFollowAnimation> GetFollowAnimation(ScreenBase const & startScreen, m2::PointD const & userPos,
                                                 double targetScale, double targetAngle, m2::PointD const & endPixelPos)
{
  auto anim = make_unique_dp<MapFollowAnimation>(userPos, startScreen.GetScale(), targetScale,
                                                 startScreen.GetAngle(), targetAngle,
                                                 startScreen.GtoP(userPos), endPixelPos, startScreen.PixelRect());
  anim->SetMaxDuration(kMaxAnimationTimeSec);

  return anim;
}

drape_ptr<MapScaleAnimation> GetScaleAnimation(ScreenBase const & startScreen, m2::PointD pxScaleCenter,
                                               m2::PointD glbScaleCenter, double factor)
{
  ScreenBase endScreen = startScreen;
  ApplyScale(pxScaleCenter, factor, endScreen);

  m2::PointD const offset = startScreen.PixelRect().Center() - startScreen.P3dtoP(pxScaleCenter);
  auto anim = make_unique_dp<MapScaleAnimation>(startScreen.GetScale(), endScreen.GetScale(),
                                                glbScaleCenter, offset);
  anim->SetMaxDuration(kMaxAnimationTimeSec);

  return anim;
}

} // namespace df

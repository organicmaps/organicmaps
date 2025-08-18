#include "drape_frontend/screen_animations.hpp"

#include "drape_frontend/animation/follow_animation.hpp"
#include "drape_frontend/animation/linear_animation.hpp"
#include "drape_frontend/animation/scale_animation.hpp"
#include "drape_frontend/animation/sequence_animation.hpp"
#include "drape_frontend/animation_constants.hpp"
#include "drape_frontend/screen_operations.hpp"

namespace df
{

std::string const kPrettyMoveAnim = "PrettyMove";
std::string const kPrettyFollowAnim = "PrettyFollow";
std::string const kParallelFollowAnim = "ParallelFollow";
std::string const kParallelLinearAnim = "ParallelLinear";

drape_ptr<SequenceAnimation> GetPrettyMoveAnimation(ScreenBase const & startScreen, ScreenBase const & endScreen)
{
  return GetPrettyMoveAnimation(startScreen, startScreen.GetScale(), endScreen.GetScale(), startScreen.GetOrg(),
                                endScreen.GetOrg());
}

drape_ptr<SequenceAnimation> GetPrettyMoveAnimation(ScreenBase const & screen, m2::AnyRectD const & startRect,
                                                    m2::AnyRectD const & endRect)
{
  double const startScale = CalculateScale(screen.PixelRect(), startRect.GetLocalRect());
  double const endScale = CalculateScale(screen.PixelRect(), endRect.GetLocalRect());

  return GetPrettyMoveAnimation(screen, startScale, endScale, startRect.GlobalCenter(), endRect.GlobalCenter());
}

drape_ptr<SequenceAnimation> GetPrettyMoveAnimation(ScreenBase const & screen, double startScale, double endScale,
                                                    m2::PointD const & startPt, m2::PointD const & endPt)
{
  double const moveDuration = PositionInterpolator::GetMoveDuration(startPt, endPt, screen.PixelRectIn3d(), startScale);
  ASSERT_GREATER(moveDuration, 0.0, ());

  double const scaleFactor = moveDuration / kMaxAnimationTimeSec * 2.0;

  auto sequenceAnim = make_unique_dp<SequenceAnimation>();
  sequenceAnim->SetCustomType(kPrettyMoveAnim);

  auto zoomOutAnim = make_unique_dp<MapLinearAnimation>();
  zoomOutAnim->SetScale(startScale, startScale * scaleFactor);
  zoomOutAnim->SetMaxDuration(kMaxAnimationTimeSec * 0.5);

  // TODO (in future): Pass fixed duration instead of screen.
  auto moveAnim = make_unique_dp<MapLinearAnimation>();
  moveAnim->SetMove(startPt, endPt, screen);
  moveAnim->SetMaxDuration(kMaxAnimationTimeSec);

  auto zoomInAnim = make_unique_dp<MapLinearAnimation>();
  zoomInAnim->SetScale(startScale * scaleFactor, endScale);
  zoomInAnim->SetMaxDuration(kMaxAnimationTimeSec * 0.5);

  sequenceAnim->AddAnimation(std::move(zoomOutAnim));
  sequenceAnim->AddAnimation(std::move(moveAnim));
  sequenceAnim->AddAnimation(std::move(zoomInAnim));
  return sequenceAnim;
}

drape_ptr<SequenceAnimation> GetPrettyFollowAnimation(ScreenBase const & startScreen, m2::PointD const & userPos,
                                                      double targetScale, double targetAngle,
                                                      m2::PointD const & endPixelPos)
{
  auto sequenceAnim = make_unique_dp<SequenceAnimation>();
  sequenceAnim->SetCustomType(kPrettyFollowAnim);

  m2::RectD const viewportRect = startScreen.PixelRectIn3d();

  ScreenBase tmp = startScreen;
  tmp.SetAngle(targetAngle);
  tmp.MatchGandP3d(userPos, viewportRect.Center());

  double const moveDuration = PositionInterpolator::GetMoveDuration(startScreen.GetOrg(), tmp.GetOrg(), startScreen);
  ASSERT_GREATER(moveDuration, 0.0, ());

  double const scaleFactor = moveDuration / kMaxAnimationTimeSec * 2.0;

  tmp = startScreen;

  if (moveDuration > 0.0)
  {
    tmp.SetScale(startScreen.GetScale() * scaleFactor);

    auto zoomOutAnim = make_unique_dp<MapLinearAnimation>();
    zoomOutAnim->SetScale(startScreen.GetScale(), tmp.GetScale());
    zoomOutAnim->SetMaxDuration(kMaxAnimationTimeSec * 0.5);
    sequenceAnim->AddAnimation(std::move(zoomOutAnim));

    tmp.MatchGandP3d(userPos, viewportRect.Center());

    auto moveAnim = make_unique_dp<MapLinearAnimation>();
    moveAnim->SetMove(startScreen.GetOrg(), tmp.GetOrg(), viewportRect, tmp.GetScale());
    moveAnim->SetMaxDuration(kMaxAnimationTimeSec);
    sequenceAnim->AddAnimation(std::move(moveAnim));
  }

  auto followAnim =
      make_unique_dp<MapFollowAnimation>(tmp, userPos, endPixelPos, targetScale, targetAngle, false /* isAutoZoom */);
  followAnim->SetMaxDuration(kMaxAnimationTimeSec * 0.5);
  sequenceAnim->AddAnimation(std::move(followAnim));
  return sequenceAnim;
}

drape_ptr<MapLinearAnimation> GetRectAnimation(ScreenBase const & startScreen, ScreenBase const & endScreen)
{
  auto anim = make_unique_dp<MapLinearAnimation>();

  anim->SetRotate(startScreen.GetAngle(), endScreen.GetAngle());
  anim->SetMove(startScreen.GetOrg(), endScreen.GetOrg(), startScreen.PixelRectIn3d(),
                (startScreen.GetScale() + endScreen.GetScale()) / 2.0);
  anim->SetScale(startScreen.GetScale(), endScreen.GetScale());
  anim->SetMaxScaleDuration(kMaxAnimationTimeSec);

  return anim;
}

drape_ptr<MapLinearAnimation> GetSetRectAnimation(ScreenBase const & screen, m2::AnyRectD const & startRect,
                                                  m2::AnyRectD const & endRect)
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
                                                 double targetScale, double targetAngle, m2::PointD const & endPixelPos,
                                                 bool isAutoZoom)
{
  auto anim =
      make_unique_dp<MapFollowAnimation>(startScreen, userPos, endPixelPos, targetScale, targetAngle, isAutoZoom);
  anim->SetMaxDuration(kMaxAnimationTimeSec);

  return anim;
}

drape_ptr<MapScaleAnimation> GetScaleAnimation(ScreenBase const & startScreen, m2::PointD pxScaleCenter,
                                               m2::PointD glbScaleCenter, double factor)
{
  ScreenBase endScreen = startScreen;
  ApplyScale(pxScaleCenter, factor, endScreen);

  auto anim =
      make_unique_dp<MapScaleAnimation>(startScreen.GetScale(), endScreen.GetScale(), glbScaleCenter, pxScaleCenter);
  anim->SetMaxDuration(kMaxAnimationTimeSec);

  return anim;
}

}  // namespace df

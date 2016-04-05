#pragma once

#include "animation/base_interpolator.hpp"

#include "drape/pointers.hpp"

#include "geometry/screenbase.hpp"

#include "std/deque.hpp"
#include "std/noncopyable.hpp"

namespace df
{

uint32_t constexpr kMoveAnimationBit = 1;
uint32_t constexpr kScaleAnimationBit = 1 << 1;
uint32_t constexpr kRotateAnimationBit = 1 << 2;
uint32_t constexpr kPerspectiveAnimationBit = 1 << 3;

uint32_t constexpr kArrowAnimObjBit = 1;
uint32_t constexpr kPlaneAnimObjBit = 1 << 1;
uint32_t constexpr kSelectionAnimBit = 1 << 2;

class Animation
{
public:
  enum Type
  {
    Sequence,
    Parallel,
    ModelView,
    Perspective,
    Arrow
  };

  enum Object
  {
    MyPositionArrow,
    MapPlane,
    Selection
  };

  Animation(bool couldBeInterrupted, bool couldBeMixed)
    : m_couldBeInterrupted(couldBeInterrupted)
    , m_couldBeMixed(couldBeMixed)
  {}

  virtual void OnStart() {}
  virtual void OnFinish() {}

  virtual Type GetType() const = 0;

  virtual uint32_t GetMask(uint32_t object) const = 0;
  virtual uint32_t GetObjects() const = 0;

  virtual void SetMaxDuration(double maxDuration) = 0;
  virtual double GetDuration() const = 0;
  virtual bool IsFinished() const = 0;

  virtual void Advance(double elapsedSeconds) = 0;

  virtual double GetScale(uint32_t object) const = 0;
  virtual double GetAngle(uint32_t object) const = 0;
  virtual m2::PointD GetPosition(uint32_t object) const = 0;

  bool CouldBeInterrupted() const { return m_couldBeInterrupted; }
  bool CouldBeMixed() const { return m_couldBeMixed; }
  bool CouldBeMixedWith(uint32_t object, int mask)
  {
    return m_couldBeMixed && !(mask & GetMask(object));
  }

protected:
  bool m_couldBeInterrupted;
  bool m_couldBeMixed;
};

class Interpolator
{
public:
  Interpolator(double duration, double delay = 0);
  virtual ~Interpolator();

  bool IsFinished() const;
  virtual void Advance(double elapsedSeconds);
  void SetMaxDuration(double maxDuration);
  double GetDuration() const;

protected:
  double GetT() const;
  double GetElapsedTime() const;

private:
  double m_elapsedTime;
  double m_duration;
  double m_delay;
};


class PositionInterpolator: public Interpolator
{
  using TBase = Interpolator;

public:
  PositionInterpolator(m2::PointD const & startPosition, m2::PointD const & endPosition, ScreenBase const & convertor);
  PositionInterpolator(double delay, m2::PointD const & startPosition, m2::PointD const & endPosition, ScreenBase const & convertor);

  static double GetMoveDuration(m2::PointD const & startPosition, m2::PointD const & endPosition, ScreenBase const & convertor);

  void Advance(double elapsedSeconds) override;
  virtual m2::PointD GetPosition() const { return m_position; }

private:
  m2::PointD m_startPosition;
  m2::PointD m_endPosition;
  m2::PointD m_position;
};

class ScaleInterpolator: public Interpolator
{
  using TBase = Interpolator;

public:
  ScaleInterpolator(double startScale, double endScale);
  ScaleInterpolator(double delay, double startScale, double endScale);

  static double GetScaleDuration(double startScale, double endScale);

  void Advance(double elapsedSeconds) override;
  virtual double GetScale() const { return m_scale; }

private:
  double const m_startScale;
  double const m_endScale;
  double m_scale;
};

class AngleInterpolator: public Interpolator
{
  using TBase = Interpolator;

public:
  AngleInterpolator(double startAngle, double endAngle);
  AngleInterpolator(double delay, double startAngle, double endAngle);

  static double GetRotateDuration(double startAngle, double endAngle);

  void Advance(double elapsedSeconds) override;
  virtual double GetAngle() const { return m_angle; }

private:
  double const m_startAngle;
  double const m_endAngle;
  double m_angle;
};

class ArrowAnimation : public Animation
{
public:
  ArrowAnimation(m2::PointD const & startPos, m2::PointD const & endPos,
                 double startAngle, double endAngle, ScreenBase const & convertor)
    : Animation(false, false)
  {
    m_positionInterpolator.reset(new PositionInterpolator(startPos, endPos, convertor));
    m_angleInterpolator.reset(new AngleInterpolator(startAngle, endAngle));
  }

  Animation::Type GetType() const override { return Animation::Arrow; }

  uint32_t GetMask(uint32_t object) const override
  {
    return (object & kArrowAnimObjBit) &&
        ((m_angleInterpolator != nullptr ? kRotateAnimationBit : 0) |
        (m_positionInterpolator != nullptr ? kMoveAnimationBit : 0));
  }

  uint32_t GetObjects() const override
  {
     return kArrowAnimObjBit;
  }

  void Advance(double elapsedSeconds) override;

private:
  drape_ptr<PositionInterpolator> m_positionInterpolator;
  drape_ptr<AngleInterpolator> m_angleInterpolator;
};

class PerspectiveSwitchAnimation : public Animation
{
  PerspectiveSwitchAnimation()
    : Animation(false, false)
  {}

  Animation::Type GetType() const override { return Animation::Perspective; }

  uint32_t GetMask(uint32_t object) const override
  {
    return (object & kPlaneAnimObjBit) &&
        (m_angleInterpolator != nullptr ? kPerspectiveAnimationBit : 0);
  }

  uint32_t GetObjects() const override
  {
     return kPlaneAnimObjBit;
  }

  void Advance(double elapsedSeconds) override;

private:
  drape_ptr<AngleInterpolator> m_angleInterpolator;
};

class FollowAnimation : public Animation
{
public:
  FollowAnimation(m2::PointD const & startPos, m2::PointD const & endPos,
                  double startAngle, double endAngle,
                  double startScale, double endScale, ScreenBase const & convertor);
  FollowAnimation();

  void SetMove(m2::PointD const & startPos, m2::PointD const & endPos, ScreenBase const & convertor);
  void SetRotate(double startAngle, double endAngle);
  void SetScale(double startScale, double endScale);

  Animation::Type GetType() const override { return Animation::ModelView; }

  uint32_t GetMask(uint32_t object) const override
  {
    return (object & kPlaneAnimObjBit) ?
        ((m_angleInterpolator != nullptr ? kPerspectiveAnimationBit : 0) |
        (m_positionInterpolator != nullptr ? kMoveAnimationBit : 0) |
        (m_scaleInterpolator != nullptr ? kScaleAnimationBit : 0)) : 0;
  }

  uint32_t GetObjects() const override
  {
     return kPlaneAnimObjBit;
  }

  void Advance(double elapsedSeconds) override;

  void SetMaxDuration(double maxDuration) override;
  double GetDuration() const override;
  bool IsFinished() const override;

  double GetScale(uint32_t object) const override;
  double GetAngle(uint32_t object) const override;
  m2::PointD GetPosition(uint32_t object) const override;

private:
  drape_ptr<AngleInterpolator> m_angleInterpolator;
  drape_ptr<PositionInterpolator> m_positionInterpolator;
  drape_ptr<ScaleInterpolator> m_scaleInterpolator;
};

using TAnimations = vector<ref_ptr<Animation>>;

class SequenceAnimation : public Animation
{
public:
  Animation::Type GetType() const override { return Animation::Sequence; }
  uint32_t GetMask(uint32_t object) const override;
  uint32_t GetObjects() const override;

  void AddAnimation(ref_ptr<Animation> animation);

  void OnStart() override;
  void OnFinish() override;

  void Advance(double elapsedSeconds) override;

private:
  deque<ref_ptr<Animation>> m_animations;
};

class ParallelAnimation : public Animation
{
public:
  Animation::Type GetType() const override { return Animation::Parallel; }
  uint32_t GetMask(uint32_t object) const override;
  uint32_t GetObjects() const override;

  void AddAnimation(ref_ptr<Animation> animation);

  void OnStart() override;
  void OnFinish() override;

  void Advance(double elapsedSeconds) override;

private:
  TAnimations m_animations;
};

class AnimationSystem : private noncopyable
{
public:
  static AnimationSystem & Instance();

  m2::AnyRectD GetRect(ScreenBase const & currentScreen);

  bool AnimationExists(Animation::Object object);

  void AddAnimation(drape_ptr<Animation> && animation);

  void Advance(double elapsedSeconds);

private:
  AnimationSystem() {}

private:

  set<drape_ptr<Animation>> m_animations;
};

}

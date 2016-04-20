#pragma once

#include "animation/base_interpolator.hpp"

#include "drape/pointers.hpp"

#include "geometry/screenbase.hpp"

#include "std/set.hpp"
#include "std/deque.hpp"
#include "std/noncopyable.hpp"

#include "boost/variant/variant_fwd.hpp"

namespace df
{

class Animation
{
public:
  enum Type
  {
    Sequence,
    Parallel,
    MapLinear,
    MapScale,
    MapFollow,
    MapPerspective,
    Arrow
  };

  enum Object
  {
    MyPositionArrow,
    MapPlane,
    Selection
  };

  enum ObjectProperty
  {
    Position,
    Scale,
    Angle
  };

  using TObject = uint32_t;
  using TProperty = uint32_t;
  using TPropValue = boost::variant<double, m2::PointD>;
  using TAnimObjects = set<TObject>;
  using TObjectProperties = set<TProperty>;

  Animation(bool couldBeInterrupted, bool couldBeMixed)
    : m_couldBeInterrupted(couldBeInterrupted)
    , m_couldBeMixed(couldBeMixed)
  {}

  virtual void OnStart() {}
  virtual void OnFinish() {}
  virtual void Interrupt() {}

  virtual Type GetType() const = 0;

  virtual TAnimObjects const & GetObjects() const = 0;
  virtual bool HasObject(TObject object) const = 0;
  virtual TObjectProperties const & GetProperties(TObject object) const = 0;
  virtual bool HasProperty(TObject object, TProperty property) const = 0;

  virtual void SetMaxDuration(double maxDuration) = 0;
  virtual double GetDuration() const = 0;
  virtual bool IsFinished() const = 0;

  virtual void Advance(double elapsedSeconds) = 0;

  virtual TPropValue GetProperty(TObject object, TProperty property) const = 0;

  bool CouldBeInterrupted() const { return m_couldBeInterrupted; }
  bool CouldBeMixed() const { return m_couldBeMixed; }
  bool CouldBeMixedWith(TObject object, TObjectProperties const & properties);
  bool CouldBeMixedWith(Animation const & animation);

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
  void SetMinDuration(double minDuration);
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
  PositionInterpolator(double duration, double delay, m2::PointD const & startPosition, m2::PointD const & endPosition);
  PositionInterpolator(m2::PointD const & startPosition, m2::PointD const & endPosition, ScreenBase const & convertor);
  PositionInterpolator(double delay, m2::PointD const & startPosition, m2::PointD const & endPosition, ScreenBase const & convertor);
  PositionInterpolator(m2::PointD const & startPosition, m2::PointD const & endPosition, m2::RectD const & pixelRect);
  PositionInterpolator(double delay, m2::PointD const & startPosition, m2::PointD const & endPosition, m2::RectD const & pixelRect);

  static double GetMoveDuration(m2::PointD const & startPosition, m2::PointD const & endPosition, ScreenBase const & convertor);
  static double GetPixelMoveDuration(m2::PointD const & startPosition, m2::PointD const & endPosition, m2::RectD const & pixelRect);

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

private:
  drape_ptr<PositionInterpolator> m_positionInterpolator;
  drape_ptr<AngleInterpolator> m_angleInterpolator;
  TAnimObjects m_objects;
  TObjectProperties m_properties;
};

class PerspectiveSwitchAnimation : public Animation
{
  PerspectiveSwitchAnimation()
    : Animation(false, false)
  {}

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

private:
  drape_ptr<AngleInterpolator> m_angleInterpolator;
  TAnimObjects m_objects;
  TObjectProperties m_properties;
};

class MapLinearAnimation : public Animation
{
public:
  MapLinearAnimation(m2::PointD const & startPos, m2::PointD const & endPos,
                  double startAngle, double endAngle,
                  double startScale, double endScale, ScreenBase const & convertor);
  MapLinearAnimation();

  void SetMove(m2::PointD const & startPos, m2::PointD const & endPos, ScreenBase const & convertor);
  void SetRotate(double startAngle, double endAngle);
  void SetScale(double startScale, double endScale);

  Animation::Type GetType() const override { return Animation::MapLinear; }

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

  void SetMaxDuration(double maxDuration) override;
  double GetDuration() const override;
  bool IsFinished() const override;

  TPropValue GetProperty(TObject object, TProperty property) const override;

private:
  drape_ptr<AngleInterpolator> m_angleInterpolator;
  drape_ptr<PositionInterpolator> m_positionInterpolator;
  drape_ptr<ScaleInterpolator> m_scaleInterpolator;
  TObjectProperties m_properties;
  TAnimObjects m_objects;
};

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

  void SetMaxDuration(double maxDuration) override;
  double GetDuration() const override;
  bool IsFinished() const override;

  TPropValue GetProperty(TObject object, TProperty property) const override;

private:
  drape_ptr<ScaleInterpolator> m_scaleInterpolator;
  m2::PointD const m_pixelOffset;
  m2::PointD const m_globalPosition;
  TObjectProperties m_properties;
  TAnimObjects m_objects;
};

class MapFollowAnimation : public Animation
{
public:
  MapFollowAnimation(m2::PointD const & globalPosition,
                     double startScale, double endScale,
                     double startAngle, double endAngle,
                     m2::PointD const & startPixelPosition, m2::PointD const & endPixelPosition,
                     m2::RectD const & pixelRect);

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

  void SetMaxDuration(double maxDuration) override;
  double GetDuration() const override;
  bool IsFinished() const override;

  TPropValue GetProperty(TObject object, TProperty property) const override;

private:
  drape_ptr<ScaleInterpolator> m_scaleInterpolator;
  drape_ptr<PositionInterpolator> m_pixelPosInterpolator;
  drape_ptr<AngleInterpolator> m_angleInterpolator;

  m2::PointD const m_globalPosition;

  TObjectProperties m_properties;
  TAnimObjects m_objects;
};

class SequenceAnimation : public Animation
{
public:
  Animation::Type GetType() const override { return Animation::Sequence; }
  TAnimObjects const & GetObjects() const override;
  bool HasObject(TObject object) const override;
  TObjectProperties const & GetProperties(TObject object) const override;
  bool HasProperty(TObject object, TProperty property) const override;

  void AddAnimation(drape_ptr<Animation> && animation);

  void OnStart() override;
  void OnFinish() override;

  void Advance(double elapsedSeconds) override;

private:
  deque<drape_ptr<Animation>> m_animations;
};

class ParallelAnimation : public Animation
{
public:
  Animation::Type GetType() const override { return Animation::Parallel; }
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

  void AddAnimation(drape_ptr<Animation> && animation);

  void OnStart() override;
  void OnFinish() override;

  void Advance(double elapsedSeconds) override;

private:
  list<drape_ptr<Animation>> m_animations;
  TAnimObjects m_objects;
  map<TObject, TObjectProperties> m_properties;
};

class AnimationSystem : private noncopyable
{
public:
  static AnimationSystem & Instance();

  m2::AnyRectD GetRect(ScreenBase const & currentScreen);

  bool AnimationExists(Animation::TObject object) const;

  void AddAnimation(drape_ptr<Animation> && animation, bool force);
  void PushAnimation(drape_ptr<Animation> && animation);

  void Advance(double elapsedSeconds);

  ScreenBase const & GetLastScreen() { return *m_lastScreen.get(); }

private:
  Animation::TPropValue GetProperty(Animation::TObject object, Animation::TProperty property, Animation::TPropValue current) const;
  void SaveAnimationResult(Animation const & animation);

  AnimationSystem();

private:
  using TAnimationList = list<drape_ptr<Animation>>;
  using TAnimationChain = deque<TAnimationList>;
  using TPropertyCache = map<pair<Animation::TObject, Animation::TProperty>, Animation::TPropValue>;
  TAnimationChain m_animationChain;
  mutable TPropertyCache m_propertyCache;

  drape_ptr<ScreenBase> m_lastScreen;
};

}

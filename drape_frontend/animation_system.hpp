#pragma once

#include "drape/pointers.hpp"

#include "geometry/screenbase.hpp"

#include "std/deque.hpp"
#include "std/noncopyable.hpp"
#include "std/unordered_set.hpp"

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
    Arrow,
    KineticScroll
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
    Angle,
    AnglePerspective,
    SwitchPerspective
  };

  struct SwitchPerspectiveParams
  {
    SwitchPerspectiveParams() = default;

    SwitchPerspectiveParams(bool enable,
                            double startAngle, double endAngle,
                            double angleFOV)
      : m_enable(enable)
      , m_startAngle(startAngle)
      , m_endAngle(endAngle)
      , m_angleFOV(angleFOV)
    {}

    bool m_enable = false;
    double m_startAngle = 0.0;
    double m_endAngle = 0.0;
    double m_angleFOV = 0.0;
  };

  struct PropertyValue
  {
    enum Type
    {
      ValueD,
      ValuePointD,
      ValuePerspectiveParams
    };

    PropertyValue()
    {}

    explicit PropertyValue(double value)
      : m_type(ValueD)
      , m_valueD(value)
    {}

    explicit PropertyValue(m2::PointD const & value)
      : m_type(ValuePointD)
      , m_valuePointD(value)
    {}

    explicit PropertyValue(SwitchPerspectiveParams const & params)
      : m_type(ValuePerspectiveParams)
    {
      m_valuePerspectiveParams = params;
    }

    Type m_type;
    union
    {
      m2::PointD m_valuePointD;
      double m_valueD;
      SwitchPerspectiveParams m_valuePerspectiveParams;
    };
  };

  using TObject = uint32_t;
  using TProperty = uint32_t;
  using TAnimObjects = unordered_set<TObject>;
  using TObjectProperties = unordered_set<TProperty>;
  using TAction = function<void(ref_ptr<Animation>)>;

  Animation(bool couldBeInterrupted, bool couldBeBlended)
    : m_couldBeInterrupted(couldBeInterrupted)
    , m_couldBeBlended(couldBeBlended)
    , m_interruptedOnCombine(false)
  {}

  virtual void OnStart() { if (m_onStartAction != nullptr) m_onStartAction(this); }
  virtual void OnFinish() { if (m_onFinishAction != nullptr) m_onFinishAction(this); }
  virtual void Interrupt() { if (m_onInterruptAction != nullptr) m_onInterruptAction(this); }

  virtual Type GetType() const = 0;

  virtual TAnimObjects const & GetObjects() const = 0;
  virtual bool HasObject(TObject object) const = 0;
  virtual TObjectProperties const & GetProperties(TObject object) const = 0;
  virtual bool HasProperty(TObject object, TProperty property) const = 0;

  virtual void SetMaxDuration(double maxDuration) = 0;
  virtual double GetDuration() const = 0;
  virtual bool IsFinished() const = 0;

  virtual void Advance(double elapsedSeconds) = 0;
  virtual void Finish() { OnFinish(); }

  virtual bool GetProperty(TObject object, TProperty property, PropertyValue & value) const = 0;

  void SetOnStartAction(TAction const & action) { m_onStartAction = action; }
  void SetOnFinishAction(TAction const & action) { m_onFinishAction = action; }
  void SetOnInterruptAction(TAction const & action) { m_onInterruptAction = action; }

  bool CouldBeInterrupted() const { return m_couldBeInterrupted; }
  bool CouldBeBlendedWith(Animation const & animation) const;
  
  void SetInterruptedOnCombine(bool enable) { m_interruptedOnCombine = enable; }
  bool GetInterruptedOnCombine() const { return m_interruptedOnCombine; }

protected:
  TAction m_onStartAction;
  TAction m_onFinishAction;
  TAction m_onInterruptAction;

  // Animation could be interrupted in case of blending impossibility.
  bool m_couldBeInterrupted;
  // Animation could be blended with other animations.
  bool m_couldBeBlended;
  // Animation must be interrupted in case of combining another animation.
  bool m_interruptedOnCombine;
};

class Interpolator
{
public:
  Interpolator(double duration, double delay = 0);
  virtual ~Interpolator(){}

  virtual void Advance(double elapsedSeconds);
  virtual void Finish();

  bool IsFinished() const;
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
  PositionInterpolator(double duration, double delay,
                       m2::PointD const & startPosition,
                       m2::PointD const & endPosition);
  PositionInterpolator(m2::PointD const & startPosition,
                       m2::PointD const & endPosition,
                       ScreenBase const & convertor);
  PositionInterpolator(double delay, m2::PointD const & startPosition,
                       m2::PointD const & endPosition,
                       ScreenBase const & convertor);
  PositionInterpolator(m2::PointD const & startPosition,
                       m2::PointD const & endPosition,
                       m2::RectD const & pixelRect);
  PositionInterpolator(double delay, m2::PointD const & startPosition,
                       m2::PointD const & endPosition,
                       m2::RectD const & pixelRect);

  static double GetMoveDuration(m2::PointD const & startPosition,
                                m2::PointD const & endPosition, ScreenBase const & convertor);
  static double GetPixelMoveDuration(m2::PointD const & startPosition,
                                     m2::PointD const & endPosition, m2::RectD const & pixelRect);

  // Interpolator overrides:
  void Advance(double elapsedSeconds) override;
  void Finish() override;

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

  // Interpolator overrides:
  void Advance(double elapsedSeconds) override;
  void Finish() override;

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
  AngleInterpolator(double delay, double duration, double startAngle, double endAngle);

  static double GetRotateDuration(double startAngle, double endAngle);

  // Interpolator overrides:
  void Advance(double elapsedSeconds) override;
  void Finish() override;

  virtual double GetAngle() const { return m_angle; }

private:
  double const m_startAngle;
  double const m_endAngle;
  double m_angle;
};

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

class PerspectiveSwitchAnimation : public Animation
{
public:
  PerspectiveSwitchAnimation(double startAngle, double endAngle, double angleFOV);

  static double GetRotateDuration(double startAngle, double endAngle);

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
  void Finish() override;

  void OnStart() override;
  void OnFinish() override;

  void SetMaxDuration(double maxDuration) override;
  double GetDuration() const override;
  bool IsFinished() const override;

  bool GetProperty(TObject object, TProperty property, PropertyValue & value) const override;

private:
  AngleInterpolator m_angleInterpolator;
  double m_startAngle;
  double m_endAngle;
  double m_angleFOV;

  bool m_isEnablePerspectiveAnim;
  mutable bool m_needPerspectiveSwitch;
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
  void Finish() override;

  void SetMaxDuration(double maxDuration) override;
  double GetDuration() const override;
  bool IsFinished() const override;

  bool GetProperty(TObject object, TProperty property, PropertyValue & value) const override;
  
  void SetMaxScaleDuration(double maxDuration);

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

private:
  double CalculateDuration() const;

  ScaleInterpolator m_scaleInterpolator;
  PositionInterpolator m_pixelPosInterpolator;
  AngleInterpolator m_angleInterpolator;

  m2::PointD const m_globalPosition;

  TObjectProperties m_properties;
  TAnimObjects m_objects;
};

class SequenceAnimation : public Animation
{
public:
  SequenceAnimation();
  Animation::Type GetType() const override { return Animation::Sequence; }
  TAnimObjects const & GetObjects() const override;
  bool HasObject(TObject object) const override;
  TObjectProperties const & GetProperties(TObject object) const override;
  bool HasProperty(TObject object, TProperty property) const override;

  void SetMaxDuration(double maxDuration) override;
  double GetDuration() const override;
  bool IsFinished() const override;

  bool GetProperty(TObject object, TProperty property, PropertyValue &value) const override;

  void AddAnimation(drape_ptr<Animation> animation);

  void OnStart() override;
  void OnFinish() override;

  void Advance(double elapsedSeconds) override;
  void Finish() override;

private:
  void ObtainObjectProperties();

  deque<drape_ptr<Animation>> m_animations;
  TAnimObjects m_objects;
  map<TObject, TObjectProperties> m_properties;
};

class ParallelAnimation : public Animation
{
public:
  ParallelAnimation();

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

  void AddAnimation(drape_ptr<Animation> animation);

  void OnStart() override;
  void OnFinish() override;

  void Advance(double elapsedSeconds) override;
  void Finish() override;

private:
  list<drape_ptr<Animation>> m_animations;
  TAnimObjects m_objects;
  map<TObject, TObjectProperties> m_properties;
};

class AnimationSystem : private noncopyable
{
public:
  static AnimationSystem & Instance();

  bool GetRect(ScreenBase const & currentScreen, m2::AnyRectD & rect);

  bool SwitchPerspective(Animation::SwitchPerspectiveParams & params);
  bool GetPerspectiveAngle(double & angle);

  bool AnimationExists(Animation::TObject object) const;
  bool HasAnimations() const;

  void CombineAnimation(drape_ptr<Animation> animation);
  void PushAnimation(drape_ptr<Animation> animation);

  void FinishAnimations(Animation::Type type, bool rewind);
  void FinishObjectAnimations(Animation::TObject object, bool rewind);

  void Advance(double elapsedSeconds);

  ScreenBase const & GetLastScreen() { return m_lastScreen; }
  void SaveAnimationResult(Animation const & animation);

private:
  bool GetProperty(Animation::TObject object, Animation::TProperty property, Animation::PropertyValue & value) const;
  void StartNextAnimations();

  AnimationSystem();

private:
  using TAnimationList = list<drape_ptr<Animation>>;
  using TAnimationChain = deque<TAnimationList>;
  using TPropertyCache = map<pair<Animation::TObject, Animation::TProperty>, Animation::PropertyValue>;

  TAnimationChain m_animationChain;
  mutable TPropertyCache m_propertyCache;

  ScreenBase m_lastScreen;
};

}

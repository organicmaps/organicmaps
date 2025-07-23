#pragma once

#include "geometry/screenbase.hpp"

namespace df
{

class Interpolator
{
public:
  Interpolator(double duration, double delay = 0);
  virtual ~Interpolator() = default;

  virtual void Advance(double elapsedSeconds);
  virtual void Finish();

  bool IsActive() const;
  void SetActive(bool active);

  bool IsFinished() const;

  static double constexpr kInvalidDuration = -1.0;
  void SetMaxDuration(double maxDuration);
  void SetMinDuration(double minDuration);
  double GetMaxDuration() const;
  double GetMinDuration() const;

  double GetDuration() const;

protected:
  double GetT() const;
  double GetElapsedTime() const;

private:
  double m_elapsedTime;
  double m_duration;
  double m_maxDuration;
  double m_minDuration;

  double m_delay;
  bool m_isActive;
};

class PositionInterpolator : public Interpolator
{
  using TBase = Interpolator;

public:
  PositionInterpolator();
  PositionInterpolator(double duration, double delay, m2::PointD const & startPosition, m2::PointD const & endPosition);

  PositionInterpolator(m2::PointD const & startPosition, m2::PointD const & endPosition, ScreenBase const & convertor);

  PositionInterpolator(double delay, m2::PointD const & startPosition, m2::PointD const & endPosition,
                       ScreenBase const & convertor);

  PositionInterpolator(m2::PointD const & startPosition, m2::PointD const & endPosition, m2::RectD const & viewportRect,
                       double scale);

  PositionInterpolator(double delay, m2::PointD const & startPosition, m2::PointD const & endPosition,
                       m2::RectD const & viewportRect, double scale);

  static double GetMoveDuration(double globalDistance, m2::RectD const & viewportRect, double scale);

  static double GetMoveDuration(m2::PointD const & startPosition, m2::PointD const & endPosition,
                                m2::RectD const & viewportRect, double scale);

  static double GetMoveDuration(m2::PointD const & startPosition, m2::PointD const & endPosition,
                                ScreenBase const & convertor);

  // Interpolator overrides:
  void Advance(double elapsedSeconds) override;
  void Finish() override;

  m2::PointD GetPosition() const { return m_position; }
  m2::PointD GetTargetPosition() const { return m_endPosition; }

private:
  m2::PointD m_startPosition;
  m2::PointD m_endPosition;
  m2::PointD m_position;
};

class ScaleInterpolator : public Interpolator
{
  using TBase = Interpolator;

public:
  ScaleInterpolator();
  ScaleInterpolator(double startScale, double endScale, bool isAutoZoom);
  ScaleInterpolator(double delay, double startScale, double endScale, bool isAutoZoom);

  static double GetScaleDuration(double startScale, double endScale, bool isAutoZoom);

  // Interpolator overrides:
  void Advance(double elapsedSeconds) override;
  void Finish() override;

  double GetScale() const { return m_scale; }
  double GetStartScale() const { return m_startScale; }
  double GetTargetScale() const { return m_endScale; }

private:
  double m_startScale;
  double m_endScale;
  double m_scale;
};

class AngleInterpolator : public Interpolator
{
  using TBase = Interpolator;

public:
  AngleInterpolator();
  AngleInterpolator(double startAngle, double endAngle);
  AngleInterpolator(double delay, double startAngle, double endAngle);
  AngleInterpolator(double delay, double duration, double startAngle, double endAngle);

  static double GetRotateDuration(double startAngle, double endAngle);

  // Interpolator overrides:
  void Advance(double elapsedSeconds) override;
  void Finish() override;

  double GetAngle() const { return m_angle; }
  double GetStartAngle() const { return m_startAngle; }
  double GetTargetAngle() const { return m_endAngle; }

private:
  double m_startAngle;
  double m_endAngle;
  double m_angle;
};

}  // namespace df

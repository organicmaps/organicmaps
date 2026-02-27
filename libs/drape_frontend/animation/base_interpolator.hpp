#pragma once

#include "base/macros.hpp"

namespace df
{
class BaseInterpolator
{
public:
  explicit BaseInterpolator(double duration, double delay = 0);
  virtual ~BaseInterpolator();

  DISALLOW_COPY_AND_MOVE(BaseInterpolator);

  bool IsFinished() const;
  virtual void Advance(double elapsedSeconds);

protected:
  double GetT() const;
  double GetElapsedTime() const;
  double GetDuration() const;

private:
  double m_elapsedTime;
  double m_duration;
  double m_delay;
};
}  // namespace df

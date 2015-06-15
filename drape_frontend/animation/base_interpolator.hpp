#pragma once

namespace df
{

class BaseInterpolator
{
public:
  BaseInterpolator(double duration, double delay = 0);
  virtual ~BaseInterpolator();

  bool IsFinished() const;
  virtual void Advance(double elapsedSeconds);

protected:
  double GetT() const;

private:
  double m_elapsedTime;
  double m_duration;
  double m_delay;
};

}

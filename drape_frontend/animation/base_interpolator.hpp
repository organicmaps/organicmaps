#pragma once

namespace df
{

class BaseInterpolator
{
public:
  BaseInterpolator(double duration);
  virtual ~BaseInterpolator();

  bool IsFinished() const;
  virtual void Advance(double elapsedSeconds);

protected:
  double GetT() const;

private:
  double m_elapsedTime;
  double m_duration;
};

}

#pragma once

namespace anim
{
  // Interface for single animation task
  class Task
  {
  public:
    virtual void OnStart(double ts) = 0;
    virtual void OnStep(double ts) = 0;
    virtual void OnEnd(double ts) = 0;
    virtual bool IsFinished() = 0;
    virtual void Finish() = 0;

    virtual ~Task() {};
  };
}

#pragma once

namespace anim
{
  // Interface for single animation task
  class Task
  {
  public:

    enum EState
    {
      EWaitStart,
      EInProgress,
      EWaitEnd
    };

  private:

    EState m_State;

  protected:

    void SetState(EState state);

  public:

    Task();
    virtual ~Task();

    EState State() const;

    virtual void OnStart(double ts);
    virtual void OnStep(double ts);
    virtual void OnEnd(double ts);

    void Finish();
    bool IsFinished() const;
  };
}

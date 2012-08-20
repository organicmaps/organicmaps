#pragma once

namespace anim
{
  // Interface for single animation task
  class Task
  {
  public:

    enum EState
    {
      EStarted,
      EInProgress,
      ECancelled,
      EEnded
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
    virtual void OnCancel(double ts);

    void Cancel();
    void End();

    bool IsCancelled() const;
    bool IsEnded() const;
  };
}

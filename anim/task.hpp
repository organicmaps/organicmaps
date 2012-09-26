#pragma once

#include "../std/map.hpp"
#include "../std/function.hpp"

namespace anim
{
  // Interface for single animation task
  class Task
  {
  public:

    typedef function<void()> TCallback;

    enum EState
    {
      EStarted,
      EInProgress,
      ECancelled,
      EEnded
    };

  private:

    EState m_State;

    map<EState, TCallback> m_callbacks;

    void PerformCallback(EState state);

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
    bool IsRunning() const;

    void SetCallback(EState state, TCallback const & cb);
  };
}

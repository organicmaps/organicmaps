#pragma once

#include "../std/map.hpp"
#include "../std/function.hpp"

#include "../base/mutex.hpp"

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

    map<EState, TCallback> m_Callbacks;

    void PerformCallback(EState state);

    threads::Mutex m_mutex;

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

    void Lock();
    void Unlock();

    void SetCallback(EState state, TCallback const & cb);
  };
}

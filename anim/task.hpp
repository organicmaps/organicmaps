#pragma once

#include "std/map.hpp"
#include "std/list.hpp"
#include "std/function.hpp"

#include "base/mutex.hpp"

namespace anim
{
  class Controller;

  // Interface for single animation task
  class Task
  {
  public:

    typedef function<void()> TCallback;

    enum EState
    {
      EReady,
      EInProgress,
      ECancelled,
      EEnded
    };

  private:

    EState m_State;

    map<EState, list<TCallback> > m_Callbacks;

    void PerformCallbacks(EState state);

    threads::Mutex m_mutex;

    Controller * m_controller;

  protected:

    void SetState(EState state);
    friend class Controller;
    void SetController(Controller * controller);

  public:

    Task();
    virtual ~Task();

    Controller * GetController() const;

    EState State() const;

    virtual void OnStart(double ts);
    virtual void OnStep(double ts);
    virtual void OnEnd(double ts);
    virtual void OnCancel(double ts);

    void Start();
    void Cancel();
    void End();

    bool IsCancelled() const;
    bool IsEnded() const;
    bool IsRunning() const;
    bool IsReady() const;

    void Lock();
    void Unlock();

    /// is this animation task animate something,
    /// which is directly changing visual appearance.
    virtual bool IsVisual() const;

    void AddCallback(EState state, TCallback const & cb);
  };
}

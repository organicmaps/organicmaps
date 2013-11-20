#pragma once

#include "message_acceptor.hpp"
#include "memory_feature_index.hpp"

#include "../map/scales_processor.hpp"
#include "../map/tiler.hpp"

#include "../platform/platform.hpp"

#include "../base/thread.hpp"
#include "../base/thread_pool.hpp"

#include "../std/set.hpp"

namespace df
{
  class Message;
  class ThreadsCommutator;
  class ReadMWMTask;

  class BackendRendererImpl : public MessageAcceptor,
                              public threads::IRoutine
  {
  public:
    BackendRendererImpl(ThreadsCommutator * commutator,
                        double visualScale,
                        int surfaceWidth,
                        int surfaceHeight);
    ~BackendRendererImpl();

  private:
    void UpdateCoverage(const ScreenBase & screen);
    void Resize(m2::RectI const & rect);
    void FinishTask(threads::IRoutine * routine);

  private:
    void CreateTask(Tiler::RectInfo const & info);
    void CancelTask(ReadMWMTask * task, bool removefromIndex, bool postToRenderer);
    void RestartExistTasks();

  private:
    ScreenBase m_currentViewport;
    set<ReadMWMTask *> m_taskIndex;

    MemoryFeatureIndex m_index;
    Tiler m_tiler;
    ScalesProcessor m_scaleProcessor;

    /////////////////////////////////////////
    //           MessageAcceptor           //
    /////////////////////////////////////////
  private:
    void AcceptMessage(Message *message);

      /////////////////////////////////////////
      //             ThreadPart              //
      /////////////////////////////////////////
  private:
    void StartThread();
    void StopThread();
    void ThreadMain();
    virtual void Do();

    void PostToRenderThreads(Message * message);

  private:
    threads::Thread m_selfThread;
    threads::ThreadPool * m_pool;
    ThreadsCommutator * m_commutator;
  };
}

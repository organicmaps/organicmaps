#pragma once

#ifndef PARALLEL_POLYGONIZER
#define PARALLEL_POLYGONIZER 1
#endif

#if PARALLEL_POLYGONIZER
#include <QSemaphore>
#include <QThreadPool>
#include <QMutex>
#include <QMutexLocker>
#endif


class MultiProducerOneConsumer
{
#if PARALLEL_POLYGONIZER
  QThreadPool m_ThreadPool;
  QSemaphore m_ThreadPoolSemaphore;
  QMutex m_EmitMutex;
#endif

public:
  MultiProducerOneConsumer(size_t tasksPerThread);

  class ITask
#if PARALLEL_POLYGONIZER
      : public QRunnable
#endif
  {
#if PARALLEL_POLYGONIZER
    MultiProducerOneConsumer * m_pParent;
#endif

  public:
    ITask() : m_pParent(0) {}

#if PARALLEL_POLYGONIZER
    void BeforeStart(MultiProducerOneConsumer * pParent);
#endif

    virtual void RunBase() = 0;
    virtual void EmitBase(void * p) = 0;

    void Emit(void * p);

#if PARALLEL_POLYGONIZER
    // Override
    virtual void run();
#endif
  };

  void RunTask(ITask * pTask);

  void Finish();
};

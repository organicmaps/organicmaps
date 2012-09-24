#include "multiproducer_oneconsumer.hpp"

#include "../base/logging.hpp"
#include "../base/macros.hpp"
#include "../base/assert.hpp"


MultiProducerOneConsumer::MultiProducerOneConsumer(size_t tasksPerThread)
#if PARALLEL_POLYGONIZER
  : m_ThreadPoolSemaphore(m_ThreadPool.maxThreadCount() * tasksPerThread)
#endif
{
#if PARALLEL_POLYGONIZER
  LOG(LINFO, ("QThreadPool threads count = ", m_ThreadPool.maxThreadCount()));
#endif
}

void MultiProducerOneConsumer::RunTask(ITask * pTask)
{
#if PARALLEL_POLYGONIZER
  ASSERT ( pTask->autoDelete(), () );
  pTask->BeforeStart(this);
  m_ThreadPool.start(pTask);

#else
  pTask->RunBase();
  delete pTask;

#endif
}

#if PARALLEL_POLYGONIZER
void MultiProducerOneConsumer::ITask::BeforeStart(MultiProducerOneConsumer * pParent)
{
  ASSERT ( pParent, () );
  m_pParent = pParent;
  m_pParent->m_ThreadPoolSemaphore.acquire();
}

void MultiProducerOneConsumer::ITask::run()
{
  RunBase();

  m_pParent->m_ThreadPoolSemaphore.release();
}
#endif

void MultiProducerOneConsumer::ITask::Emit(void * p)
{
#if PARALLEL_POLYGONIZER
  ASSERT ( m_pParent, () );
  QMutexLocker mutexLocker(&m_pParent->m_EmitMutex);
  UNUSED_VALUE(mutexLocker);
#endif

  EmitBase(p);
}

void MultiProducerOneConsumer::Finish()
{
#if PARALLEL_POLYGONIZER
  m_ThreadPool.waitForDone();
#endif
}

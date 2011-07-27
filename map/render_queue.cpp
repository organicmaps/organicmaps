#include "../base/SRC_FIRST.hpp"

#include "render_queue.hpp"

#include "../yg/render_state.hpp"
#include "../yg/rendercontext.hpp"

#include "../base/logging.hpp"

RenderQueue::RenderQueue(
    string const & skinName,
    bool isBenchmarking,
    unsigned scaleEtalonSize,
    unsigned maxTilesCount,
    unsigned tasksCount,
    yg::Color const & bgColor
  ) : m_sequenceID(0), m_tileCache(maxTilesCount - 1)
{
  m_tasksCount = tasksCount; //< calculate from the CPU Cores Number
  LOG(LINFO, ("initializing ", tasksCount, " rendering threads"));
  m_tasks = new Task[m_tasksCount];
  for (unsigned i = 0; i < m_tasksCount; ++i)
    m_tasks[i].m_routine = new RenderQueueRoutine(
                                    skinName,
                                    isBenchmarking,
                                    scaleEtalonSize,
                                    bgColor,
                                    i,
                                    this);
}

void RenderQueue::Initialize(shared_ptr<yg::gl::RenderContext> const & primaryContext,
                             shared_ptr<yg::ResourceManager> const & resourceManager,
                             double visualScale)
{
  m_resourceManager = resourceManager;
  for (unsigned i = 0; i < m_tasksCount; ++i)
  {
    m_tasks[i].m_routine->InitializeGL(primaryContext->createShared(),
                                       m_resourceManager,
                                       visualScale);
    m_tasks[i].m_thread.Create(m_tasks[i].m_routine);
  }
}

RenderQueue::~RenderQueue()
{
  m_renderCommands.Cancel();
  for (unsigned i = 0; i < m_tasksCount; ++i)
    m_tasks[i].m_thread.Cancel();
  delete [] m_tasks;
}

void RenderQueue::AddCommand(RenderQueueRoutine::TRenderFn const & fn, yg::Tiler::RectInfo const & rectInfo, size_t sequenceID)
{
  m_sequenceID = sequenceID;
  m_renderCommands.PushBack(make_shared_ptr(new RenderQueueRoutine::Command(rectInfo, fn, sequenceID)));
}

void RenderQueue::AddWindowHandle(shared_ptr<WindowHandle> const & windowHandle)
{
  for (unsigned i = 0; i < m_tasksCount; ++i)
    m_tasks[i].m_routine->AddWindowHandle(windowHandle);
}

void RenderQueue::MemoryWarning()
{
  for (unsigned i = 0; i < m_tasksCount; ++i)
    m_tasks[i].m_routine->MemoryWarning();
}

void RenderQueue::EnterBackground()
{
  for (unsigned i = 0; i < m_tasksCount; ++i)
    m_tasks[i].m_routine->EnterBackground();
}

void RenderQueue::EnterForeground()
{
  for (unsigned i = 0; i < m_tasksCount; ++i)
    m_tasks[i].m_routine->EnterForeground();
}

yg::TileCache & RenderQueue::TileCache()
{
  return m_tileCache;
}

size_t RenderQueue::CurrentSequence() const
{
  return m_sequenceID;
}

ThreadedList<shared_ptr<RenderQueueRoutine::Command > > & RenderQueue::RenderCommands()
{
  return m_renderCommands;
}


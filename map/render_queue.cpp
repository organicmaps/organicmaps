#include "../base/SRC_FIRST.hpp"

#include "render_queue.hpp"
#include "window_handle.hpp"

#include "../yg/render_state.hpp"
#include "../yg/rendercontext.hpp"

#include "../std/bind.hpp"

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

void RenderQueue::AddCommand(
  RenderQueueRoutine::TRenderFn const & renderFn,
  Tiler::RectInfo const & rectInfo,
  size_t sequenceID,
  RenderQueueRoutine::TCommandFinishedFn const & commandFinishedFn)
{
  m_sequenceID = sequenceID;
  m_renderCommands.PushBack(make_shared_ptr(new RenderQueueRoutine::Command(renderFn, rectInfo, sequenceID, commandFinishedFn)));
}

void RenderQueue::AddWindowHandle(shared_ptr<WindowHandle> const & window)
{
  m_windowHandles.push_back(window);
}

void RenderQueue::Invalidate()
{
  for_each(m_windowHandles.begin(),
           m_windowHandles.end(),
           bind(&WindowHandle::invalidate, _1));
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

TileCache & RenderQueue::GetTileCache()
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


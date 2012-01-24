#include "queued_render_policy.hpp"
#include "events.hpp"
#include "../yg/internal/opengl.hpp"

QueuedRenderPolicy::QueuedRenderPolicy(int pipelinesCount,
                                       shared_ptr<yg::gl::RenderContext> const & primaryRC,
                                       bool doSupportsRotation)
  : RenderPolicy(primaryRC, doSupportsRotation)
{
  m_Pipelines = new PacketsPipeline[pipelinesCount];
  m_PipelinesCount = pipelinesCount;
}

QueuedRenderPolicy::~QueuedRenderPolicy()
{
/*  for (unsigned i = 0; i < m_PipelinesCount; ++i)
    CancelQueuedCommands(i);*/

  delete [] m_Pipelines;

  LOG(LINFO, ("deleted QueuedRenderPolicy"));
}

bool QueuedRenderPolicy::NeedRedraw() const
{
  if (RenderPolicy::NeedRedraw())
    return true;

  for (unsigned i = 0; i < m_PipelinesCount; ++i)
    if (!m_Pipelines[i].m_Queue.empty())
      return true;

  return false;
}

void QueuedRenderPolicy::BeginFrame(shared_ptr<PaintEvent> const & ev, ScreenBase const & s)
{
  m_IsDebugging = false;
  if (m_IsDebugging)
    LOG(LINFO, ("-------BeginFrame-------"));
  base_t::BeginFrame(ev, s);
}

void QueuedRenderPolicy::EndFrame(shared_ptr<PaintEvent> const & ev, ScreenBase const & s)
{
  base_t::EndFrame(ev, s);
  if (m_IsDebugging)
    LOG(LINFO, ("-------EndFrame-------"));
}

void QueuedRenderPolicy::DrawFrame(shared_ptr<PaintEvent> const & ev, ScreenBase const & s)
{
  for (unsigned i = 0; i < m_PipelinesCount; ++i)
  {
    RenderQueuedCommands(i);
    m_resourceManager->updatePoolState();
  }
}

void QueuedRenderPolicy::RenderQueuedCommands(int pipelineNum)
{
  /// logging only calls that is made while rendering tiles.
//  if ((pipelineNum == 0) && (m_IsDebugging))
//    yg::gl::g_doLogOGLCalls = true;

  if (m_IsDebugging)
    LOG(LINFO, ("--- Processing Pipeline #", pipelineNum, " ---"));

  unsigned cmdProcessed = 0;

  m_Pipelines[pipelineNum].m_Queue.processList(bind(&QueuedRenderPolicy::PacketsPipeline::FillFrameCommands, &m_Pipelines[pipelineNum], _1, 1));

  cmdProcessed = m_Pipelines[pipelineNum].m_FrameCommands.size();

  list<yg::gl::Packet>::iterator it;

  yg::gl::Packet::EType bucketType = m_Pipelines[pipelineNum].m_Type;

  for (it = m_Pipelines[pipelineNum].m_FrameCommands.begin();
       it != m_Pipelines[pipelineNum].m_FrameCommands.end();
       ++it)
  {
    if (it->m_command)
      it->m_command->setIsDebugging(m_IsDebugging);

    if (bucketType == yg::gl::Packet::ECancelPoint)
    {
      if (it->m_command)
        it->m_command->cancel();
    }
    else
    {
      ASSERT(bucketType == yg::gl::Packet::ECheckPoint, ());

      if (it->m_command)
        it->m_command->perform();
    }
  }

  /// should clear to release resources, refered from the stored commands.
  m_Pipelines[pipelineNum].m_FrameCommands.clear();

  if (m_IsDebugging)
  {
    LOG(LINFO, ("processed", cmdProcessed, "commands"));
    LOG(LINFO, (m_Pipelines[pipelineNum].m_Queue.size(), "commands left"));
  }

//  if ((pipelineNum == 0) && (m_IsDebugging))
//    yg::gl::g_doLogOGLCalls = false;
}

void QueuedRenderPolicy::PacketsPipeline::FillFrameCommands(list<yg::gl::Packet> & renderQueue, int maxFrames)
{
  m_FrameCommands.clear();

  /// searching for "delimiter" markers

  list<yg::gl::Packet>::iterator first = renderQueue.begin();
  list<yg::gl::Packet>::iterator last = renderQueue.begin();

  /// checking whether there are a CancelPoint packet in the queue.
  /// In this case - fill m_FrameCommands till this packet

  for (list<yg::gl::Packet>::iterator it = renderQueue.begin();
       it != renderQueue.end();
       ++it)
  {
    yg::gl::Packet p = *it;
    if (p.m_type == yg::gl::Packet::ECancelPoint)
    {
      copy(first, ++it, back_inserter(m_FrameCommands));
      renderQueue.erase(first, it);
      m_Type = p.m_type;
      return;
    }
  }

  /// we know, that there are no CancelPoint packets in the queue.
  /// so fill up the m_FrameCommands up to maxFrames frames.

  int packetsLeft = 100000;
  int framesLeft = maxFrames;

  while ((framesLeft != 0) && (packetsLeft != 0) && (last != renderQueue.end()))
  {
    yg::gl::Packet p = *last;

    if (p.m_type == yg::gl::Packet::ECheckPoint)
    {
      /// found frame boundary, copying
      copy(first, ++last, back_inserter(m_FrameCommands));
      /// erasing from the main queue
      renderQueue.erase(first, last);
      first = renderQueue.begin();
      last = renderQueue.begin();
      --framesLeft;
      m_Type = p.m_type;
    }
    else
      ++last;

    --packetsLeft;
  }
}

void QueuedRenderPolicy::CopyQueuedCommands(list<yg::gl::Packet> &l, list<yg::gl::Packet> &r)
{
  swap(l, r);
}

void QueuedRenderPolicy::CancelQueuedCommands(int pipelineNum)
{
  if (m_IsDebugging)
    LOG(LINFO, ("cancelling packetsQueue for pipeline", pipelineNum));

  m_Pipelines[pipelineNum].m_Queue.cancel();

  list<yg::gl::Packet> l;

  m_Pipelines[pipelineNum].m_Queue.processList(bind(&QueuedRenderPolicy::CopyQueuedCommands, this, _1, ref(l)));

  for (list<yg::gl::Packet>::iterator it = l.begin(); it != l.end(); ++it)
  {
    yg::gl::Packet p = *it;

    if (p.m_command)
      p.m_command->cancel();
  }
}

yg::gl::PacketsQueue * QueuedRenderPolicy::GetPacketsQueue(int pipelineNum)
{
  return &m_Pipelines[pipelineNum].m_Queue;
}

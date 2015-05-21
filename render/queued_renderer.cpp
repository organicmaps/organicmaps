#include "queued_renderer.hpp"

#include "graphics/opengl/opengl.hpp"

QueuedRenderer::QueuedRenderer(int pipelinesCount,
                               shared_ptr<graphics::RenderContext> const & rc)
{
  m_Pipelines = new PacketsPipeline[pipelinesCount];
  for (int i = 0; i < pipelinesCount; ++i)
    m_Pipelines[i].m_CouldExecutePartially = false;
  m_PipelinesCount = pipelinesCount;
  m_CurrentPipeline = 0;
  m_RenderContext = rc;
}

QueuedRenderer::~QueuedRenderer()
{
/*  for (unsigned i = 0; i < m_PipelinesCount; ++i)
    CancelQueuedCommands(i);*/

  delete [] m_Pipelines;

  LOG(LDEBUG, ("deleted QueuedRenderPolicy"));
}

bool QueuedRenderer::NeedRedraw() const
{
  for (unsigned i = 0; i < m_PipelinesCount; ++i)
  {
    if (!m_Pipelines[i].m_Queue.empty())
      return true;
    if (!m_Pipelines[i].m_FrameCommands.empty())
      return true;
  }

  return false;
}

void QueuedRenderer::SetPartialExecution(int pipelineNum, bool flag)
{
  m_Pipelines[pipelineNum].m_CouldExecutePartially = flag;
}

void QueuedRenderer::BeginFrame()
{
  m_IsDebugging = false;
}

void QueuedRenderer::EndFrame()
{
}

void QueuedRenderer::DrawFrame()
{
  /// cyclically checking pipelines starting from m_CurrentPipeline
  for (unsigned i = 0; i < m_PipelinesCount; ++i)
  {
    int num = (m_CurrentPipeline + i) % m_PipelinesCount;

    if (RenderQueuedCommands(num))
    {
      /// next DrawFrame should start from another pipeline
      m_CurrentPipeline = (num + 1) % m_PipelinesCount;
    }
  }
}

bool QueuedRenderer::RenderQueuedCommands(int pipelineNum)
{
  /// logging only calls that is made while rendering tiles.
//  if ((pipelineNum == 0) && (m_IsDebugging))
//    graphics::gl::g_doLogOGLCalls = true;

  unsigned cmdProcessed = 0;

  /// FrameCommands could contain commands from the previous frame if
  /// the processed pipeline is allowed to be executed partially.
  PacketsPipeline & ppl = m_Pipelines[pipelineNum];
  if (ppl.m_FrameCommands.empty())
    ppl.m_Queue.processList([&ppl] (list<graphics::Packet> & queueData) { ppl.FillFrameCommands(queueData, 1); });

  cmdProcessed = ppl.m_FrameCommands.size();

  list<graphics::Packet>::iterator it;

  bool res = !ppl.m_FrameCommands.empty();
  bool partialExecution = ppl.m_CouldExecutePartially;

  graphics::Packet::EType bucketType = ppl.m_Type;

  while (!ppl.m_FrameCommands.empty())
  {
    it = ppl.m_FrameCommands.begin();
    if (it->m_command)
    {
      it->m_command->setRenderContext(m_RenderContext.get());
      it->m_command->setIsDebugging(m_IsDebugging);
    }

    if (bucketType == graphics::Packet::ECancelPoint)
    {
      if (it->m_command)
        it->m_command->cancel();
    }
    else
    {
      ASSERT(bucketType == graphics::Packet::EFramePoint, ());

      if (it->m_command)
        it->m_command->perform();
    }

    bool isCheckpoint = (it->m_type == graphics::Packet::ECheckPoint);

    ppl.m_FrameCommands.pop_front();

    /// if we found a checkpoint instead of frameboundary and this
    /// pipeline is allowed to be partially executed we are
    /// breaking from processing cycle.
    if (isCheckpoint && partialExecution)
      break;
  }

  return res;

//  if ((pipelineNum == 0) && (m_IsDebugging))
//    graphics::gl::g_doLogOGLCalls = false;
}

void QueuedRenderer::PacketsPipeline::FillFrameCommands(list<graphics::Packet> & renderQueue, int maxFrames)
{
  ASSERT(m_FrameCommands.empty(), ());

  /// searching for "delimiter" markers

  list<graphics::Packet>::iterator first = renderQueue.begin();
  list<graphics::Packet>::iterator last = renderQueue.begin();

  /// checking whether there are a CancelPoint packet in the queue.
  /// In this case - fill m_FrameCommands till this packet

  for (list<graphics::Packet>::iterator it = renderQueue.begin();
       it != renderQueue.end();
       ++it)
  {
    graphics::Packet p = *it;
    if (p.m_type == graphics::Packet::ECancelPoint)
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
    graphics::Packet p = *last;

    if (p.m_type == graphics::Packet::EFramePoint)
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

void QueuedRenderer::CopyQueuedCommands(list<graphics::Packet> & l, list<graphics::Packet> & r)
{
  swap(l, r);
}

void QueuedRenderer::CancelQueuedCommands(int pipelineNum)
{
  m_Pipelines[pipelineNum].m_Queue.cancel();

  list<graphics::Packet> r;

  m_Pipelines[pipelineNum].m_Queue.processList([this, &r] (list<graphics::Packet> & l) { CopyQueuedCommands(l, r); });

  for (graphics::Packet const & p : r)
  {
    if (p.m_command)
      p.m_command->cancel();
  }
}

graphics::PacketsQueue * QueuedRenderer::GetPacketsQueue(int pipelineNum)
{
  return &m_Pipelines[pipelineNum].m_Queue;
}

void QueuedRenderer::PrepareQueueCancellation(int pipelineNum)
{
  m_Pipelines[pipelineNum].m_Queue.cancelFences();
}

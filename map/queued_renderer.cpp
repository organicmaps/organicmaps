#include "queued_renderer.hpp"

#include "../graphics/opengl/opengl.hpp"

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

  LOG(LINFO, ("deleted QueuedRenderPolicy"));
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
  if (m_IsDebugging)
    LOG(LINFO, ("-------BeginFrame-------"));
}

void QueuedRenderer::EndFrame()
{
  if (m_IsDebugging)
    LOG(LINFO, ("-------EndFrame-------"));
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

  if (m_IsDebugging)
    LOG(LINFO, ("--- Processing Pipeline #", pipelineNum, " ---"));

  unsigned cmdProcessed = 0;

  /// FrameCommands could contain commands from the previous frame if
  /// the processed pipeline is allowed to be executed partially.
  if (m_Pipelines[pipelineNum].m_FrameCommands.empty())
    m_Pipelines[pipelineNum].m_Queue.processList(bind(&QueuedRenderer::PacketsPipeline::FillFrameCommands, &m_Pipelines[pipelineNum], _1, 1));

  cmdProcessed = m_Pipelines[pipelineNum].m_FrameCommands.size();

  list<graphics::Packet>::iterator it;

  bool res = !m_Pipelines[pipelineNum].m_FrameCommands.empty();
  bool partialExecution = m_Pipelines[pipelineNum].m_CouldExecutePartially;

  graphics::Packet::EType bucketType = m_Pipelines[pipelineNum].m_Type;

  while (!m_Pipelines[pipelineNum].m_FrameCommands.empty())
  {
    it = m_Pipelines[pipelineNum].m_FrameCommands.begin();
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

    m_Pipelines[pipelineNum].m_FrameCommands.pop_front();

    /// if we found a checkpoint instead of frameboundary and this
    /// pipeline is allowed to be partially executed we are
    /// breaking from processing cycle.
    if (isCheckpoint && partialExecution)
      break;
  }

  if (m_IsDebugging)
  {
    LOG(LINFO, ("processed", cmdProcessed, "commands"));
    LOG(LINFO, (m_Pipelines[pipelineNum].m_Queue.size(), "commands left"));
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

void QueuedRenderer::CopyQueuedCommands(list<graphics::Packet> &l, list<graphics::Packet> &r)
{
  swap(l, r);
}

void QueuedRenderer::CancelQueuedCommands(int pipelineNum)
{
  if (m_IsDebugging)
    LOG(LINFO, ("cancelling packetsQueue for pipeline", pipelineNum));

  m_Pipelines[pipelineNum].m_Queue.cancel();

  list<graphics::Packet> l;

  m_Pipelines[pipelineNum].m_Queue.processList(bind(&QueuedRenderer::CopyQueuedCommands, this, _1, ref(l)));

  for (list<graphics::Packet>::iterator it = l.begin(); it != l.end(); ++it)
  {
    graphics::Packet p = *it;

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

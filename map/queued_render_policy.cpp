#include "queued_render_policy.hpp"
#include "events.hpp"

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
  for (unsigned i = 0; i < m_PipelinesCount; ++i)
    DismissQueuedCommands(i);

  delete [] m_Pipelines;

  LOG(LINFO, ("resetting state"));
  m_state.reset();
}

bool QueuedRenderPolicy::NeedRedraw() const
{
  if (RenderPolicy::NeedRedraw())
    return true;

  for (unsigned i = 0; i < m_PipelinesCount; ++i)
    if (!m_Pipelines[i].m_Queue.Empty())
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
  if (!m_state)
  {
    m_state = ev->drawer()->screen()->createState();
    m_state->m_isDebugging = m_IsDebugging;
  }

  ev->drawer()->screen()->getState(m_state.get());

  for (unsigned i = 0; i < m_PipelinesCount; ++i)
  {
    RenderQueuedCommands(i);
    m_resourceManager->mergeFreeResources();
  }
}

void QueuedRenderPolicy::RenderQueuedCommands(int pipelineNum)
{
  shared_ptr<yg::gl::BaseState> curState = m_state;

  unsigned cmdProcessed = 0;

  m_Pipelines[pipelineNum].m_Queue.ProcessList(bind(&QueuedRenderPolicy::PacketsPipeline::FillFrameBucket, &m_Pipelines[pipelineNum], _1));

  cmdProcessed = m_Pipelines[pipelineNum].m_FrameBucket.size();

  list<yg::gl::Packet>::iterator it;

  yg::gl::Packet::EType bucketType = m_Pipelines[pipelineNum].m_Type;

  for (it = m_Pipelines[pipelineNum].m_FrameBucket.begin();
       it != m_Pipelines[pipelineNum].m_FrameBucket.end();
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

      if (it->m_state)
      {
        it->m_state->m_isDebugging = m_IsDebugging;
        it->m_state->apply(curState.get());
        curState = it->m_state;
      }
      if (it->m_command)
        it->m_command->perform();
    }
  }

  /// should clear to release resources, refered from the stored commands.
  m_Pipelines[pipelineNum].m_FrameBucket.clear();

  if (m_IsDebugging)
  {
    LOG(LINFO, ("processed", cmdProcessed, "commands"));
    LOG(LINFO, (m_Pipelines[pipelineNum].m_Queue.Size(), "commands left"));
  }

  m_state->apply(curState.get());
}

void QueuedRenderPolicy::PacketsPipeline::FillFrameBucket(list<yg::gl::Packet> & renderQueue)
{
  m_FrameBucket.clear();

  /// searching for "delimiter" markers

  list<yg::gl::Packet>::iterator first = renderQueue.begin();
  list<yg::gl::Packet>::iterator last = renderQueue.begin();

  int packetsLeft = 100000;

  while ((packetsLeft != 0) && (last != renderQueue.end()))
  {
    yg::gl::Packet p = *last;
    if ((p.m_type == yg::gl::Packet::ECheckPoint)
     || (p.m_type == yg::gl::Packet::ECancelPoint))
    {
      /// found frame boundary, copying
      copy(first, ++last, back_inserter(m_FrameBucket));
      /// erasing from the main queue
      renderQueue.erase(first, last);
      first = renderQueue.begin();
      last = renderQueue.begin();
      m_Type = p.m_type;
      if (m_Type == yg::gl::Packet::ECancelPoint)
        break;
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

void QueuedRenderPolicy::DismissQueuedCommands(int pipelineNum)
{
  if (m_IsDebugging)
    LOG(LINFO, ("cancelling packetsQueue for pipeline", pipelineNum));

  m_Pipelines[pipelineNum].m_Queue.cancel();

  list<yg::gl::Packet> l;

  m_Pipelines[pipelineNum].m_Queue.ProcessList(bind(&QueuedRenderPolicy::CopyQueuedCommands, this, _1, ref(l)));

  for (list<yg::gl::Packet>::iterator it = l.begin(); it != l.end(); ++it)
  {
    yg::gl::Packet p = *it;

    if ((p.m_type == yg::gl::Packet::ECheckPoint)
     || (p.m_type == yg::gl::Packet::ECancelPoint))
    {
      if (p.m_command)
        p.m_command->perform();
    }
  }
}

yg::gl::PacketsQueue * QueuedRenderPolicy::GetPacketsQueue(int pipelineNum)
{
  return &m_Pipelines[pipelineNum].m_Queue;
}

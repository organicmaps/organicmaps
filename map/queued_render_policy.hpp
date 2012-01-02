#pragma once

#include "render_policy.hpp"
#include "../base/threaded_list.hpp"
#include "../yg/renderer.hpp"

/// base class for policy using separate queue of gl commands.
class QueuedRenderPolicy : public RenderPolicy
{
private:

  typedef RenderPolicy base_t;

  struct PacketsPipeline
  {
    yg::gl::PacketsQueue m_Queue; //< all enqueued commands
    list<yg::gl::Packet> m_FrameBucket; //< list of commands to execute on current frame

    void FillFrameBucket(list<yg::gl::Packet> & QueueData);
  };

  /// couldn't use vector here as PacketsPipeline holds non-copyable yg::gl::PacketsQueue
  PacketsPipeline * m_Pipelines;
  int m_PipelinesCount;

  bool m_IsDebugging;

  shared_ptr<yg::gl::BaseState> m_state;

protected:

  void RenderQueuedCommands(int pipelineNum);
  void DismissQueuedCommands(int pipelineNum);

public:

  QueuedRenderPolicy(int pipelinesCount,
                     shared_ptr<yg::gl::RenderContext> const & primaryRC,
                     bool doSupportsRotation);

  ~QueuedRenderPolicy();

  void BeginFrame(shared_ptr<PaintEvent> const & ev, ScreenBase const & s);
  void DrawFrame(shared_ptr<PaintEvent> const & ev, ScreenBase const & s);
  void EndFrame(shared_ptr<PaintEvent> const & ev, ScreenBase const & s);

  bool NeedRedraw() const;

  yg::gl::PacketsQueue * GetPacketsQueue(int pipelineNum);
};

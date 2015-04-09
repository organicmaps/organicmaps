#pragma once

#include "base/threaded_list.hpp"
#include "graphics/opengl/renderer.hpp"

namespace graphics
{
  class RenderContext;
}

/// Mixture-class for rendering policies, used on the
/// devices that do not support OpenGL context sharing
class QueuedRenderer
{
private:

  /// separate pipeline for packets, collected on the single thread.
  /// although it's possible to collect all the packet from all threads
  /// into single queue it's better to separate them for finer optimization
  /// of "heavy" commands.
  struct PacketsPipeline
  {
    graphics::PacketsQueue m_Queue; //< all enqueued commands
    list<graphics::Packet> m_FrameCommands; //< list of commands to execute on current frame
    graphics::Packet::EType m_Type; //< type of the actions to perform with FrameCommands

    bool m_CouldExecutePartially;

    /// - this function is passed to ThreadedList::ProcessQueue to fill up
    /// the FrameCommands from the QueueData, taking at maximum maxCheckPoints chunks,
    /// skipping empty frames.
    /// - if there are a CancelPoint in the QueueData than the packets are copied up to
    ///   CancelPoint packet ignoring maxCheckPoints param
    void FillFrameCommands(list<graphics::Packet> & QueueData, int maxCheckPoints);
  };

  /// couldn't use vector here as PacketsPipeline holds non-copyable graphics::PacketsQueue
  PacketsPipeline * m_Pipelines;
  int m_PipelinesCount;

  /// DrawFrame process only one pipeline at a frame to provide a
  /// consistent and smooth GUI experience, so to avoid a non-primary
  /// pipeline starvation we should select them in a cyclic manner
  int m_CurrentPipeline;

  bool m_IsDebugging;

  shared_ptr<graphics::RenderContext> m_RenderContext;

public:

  QueuedRenderer(int pipelinesCount, shared_ptr<graphics::RenderContext> const & rc);
  ~QueuedRenderer();

  void CopyQueuedCommands(list<graphics::Packet> & l, list<graphics::Packet> & r);

  bool RenderQueuedCommands(int pipelineNum);
  void CancelQueuedCommands(int pipelineNum);
  void PrepareQueueCancellation(int pipelineNum);
  void SetPartialExecution(int pipelineNum, bool flag);

  void BeginFrame();
  void DrawFrame();
  void EndFrame();

  bool NeedRedraw() const;

  graphics::PacketsQueue * GetPacketsQueue(int pipelineNum);
};

#pragma once

#include "../base/threaded_list.hpp"
#include "../yg/renderer.hpp"

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
    yg::gl::PacketsQueue m_Queue; //< all enqueued commands
    list<yg::gl::Packet> m_FrameCommands; //< list of commands to execute on current frame
    yg::gl::Packet::EType m_Type; //< type of the actions to perform with FrameCommands

    /// - this function is passed to ThreadedList::ProcessQueue to fill up
    /// the FrameCommands from the QueueData, taking at maximum maxCheckPoints chunks,
    /// skipping empty frames.
    /// - if there are a CancelPoint in the QueueData than the packets are copied up to
    ///   CancelPoint packet ignoring maxCheckPoints param
    void FillFrameCommands(list<yg::gl::Packet> & QueueData, int maxCheckPoints);
  };

  /// couldn't use vector here as PacketsPipeline holds non-copyable yg::gl::PacketsQueue
  PacketsPipeline * m_Pipelines;
  int m_PipelinesCount;

  /// DrawFrame process only one pipeline at a frame to provide a
  /// consistent and smooth GUI experience, so to avoid a non-primary
  /// pipeline starvation we should select them in a cyclic manner
  int m_CurrentPipeline;

  bool m_IsDebugging;

public:

  QueuedRenderer(int pipelinesCount);
  ~QueuedRenderer();

  void CopyQueuedCommands(list<yg::gl::Packet> & l, list<yg::gl::Packet> & r);

  bool RenderQueuedCommands(int pipelineNum);
  void CancelQueuedCommands(int pipelineNum);
  void PrepareQueueCancellation(int pipelineNum);

  void BeginFrame();
  void DrawFrame();
  void EndFrame();

  bool NeedRedraw() const;

  yg::gl::PacketsQueue * GetPacketsQueue(int pipelineNum);
};

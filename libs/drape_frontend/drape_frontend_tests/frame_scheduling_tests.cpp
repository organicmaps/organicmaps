#include "testing/testing.hpp"

#include "drape_frontend/base_renderer.hpp"
#include "drape_frontend/threads_commutator.hpp"

#include "drape/drape_tests/testing_graphics_context.hpp"

#include <atomic>
#include <chrono>
#include <future>

namespace frame_scheduling_tests
{
using namespace std::chrono_literals;

class TestFactory : public dp::GraphicsContextFactory
{
public:
  dp::GraphicsContext * GetDrawContext() override { return &m_context; }
  dp::GraphicsContext * GetResourcesUploadContext() override { return &m_context; }

private:
  TestingGraphicsContext m_context;
};

class TestRenderer : public df::BaseRenderer
{
public:
  using BaseRenderer::CancelMessageWaiting;
  using BaseRenderer::ProcessFrameFailure;

  TestRenderer(df::ThreadsCommutator & commutator, TestFactory & factory, dp::FrameStatus status)
    : BaseRenderer(df::ThreadsCommutator::RenderThread,
                   Params(dp::ApiVersion::OpenGLES3, make_ref(&commutator), make_ref(&factory), nullptr, {}))
    , m_status(status)
  {}

  ~TestRenderer() override { Stop(); }

  void Start()
  {
    m_started = true;
    StartThread();
  }

  void Stop()
  {
    if (!m_started)
      return;
    StopThread();
    m_started = false;
  }

  void ResumeSurface()
  {
    m_status = dp::FrameStatus::Ready;
    CancelMessageWaiting();
  }

  std::atomic<unsigned> m_attempts = 0;
  std::promise<void> m_firstAttempt;
  std::promise<void> m_frameReady;

private:
  class Routine : public threads::IRoutine
  {
  public:
    explicit Routine(TestRenderer & renderer) : m_renderer(renderer) {}
    void Do() override
    {
      while (!IsCancelled())
        m_renderer.IterateRenderLoop();
    }

  private:
    TestRenderer & m_renderer;
  };

  std::unique_ptr<threads::IRoutine> CreateRoutine() override { return std::make_unique<Routine>(*this); }
  void OnContextCreate() override {}
  void OnContextDestroy() override {}
  void AcceptMessage(ref_ptr<df::Message>) override {}

  void RenderFrame() override
  {
    if (!IsRenderingEnabled())
      return;
    auto const status = m_status.load();
    if (++m_attempts == 1)
      m_firstAttempt.set_value();
    if (status != dp::FrameStatus::Ready)
    {
      // A rebuilt swapchain or transient failure can recover without a platform event.
      if (status != dp::FrameStatus::Suspended)
        m_status = dp::FrameStatus::Ready;
      ProcessFrameFailure(status);
      return;
    }

    if (!m_rendered)
    {
      m_rendered = true;
      m_frameReady.set_value();
    }
    ProcessSingleMessage();
  }

  std::atomic<dp::FrameStatus> m_status;
  bool m_started = false;
  bool m_rendered = false;
};

UNIT_TEST(FrameScheduling_SurfaceResumeBeforeOrDuringWait)
{
  df::ThreadsCommutator commutator;
  TestFactory factory;
  TestRenderer renderer(commutator, factory, dp::FrameStatus::Suspended);
  auto attempted = renderer.m_firstAttempt.get_future();
  auto ready = renderer.m_frameReady.get_future();
  renderer.Start();
  TEST(attempted.wait_for(5s) == std::future_status::ready, ());

  renderer.ResumeSurface();
  TEST(ready.wait_for(5s) == std::future_status::ready, ());
}

UNIT_TEST(FrameScheduling_SuspendedSurfaceDoesNotPoll)
{
  df::ThreadsCommutator commutator;
  TestFactory factory;
  TestRenderer renderer(commutator, factory, dp::FrameStatus::Suspended);
  auto attempted = renderer.m_firstAttempt.get_future();
  auto ready = renderer.m_frameReady.get_future();
  renderer.Start();
  TEST(attempted.wait_for(5s) == std::future_status::ready, ());
  TEST(ready.wait_for(100ms) == std::future_status::timeout, ());
  TEST_EQUAL(renderer.m_attempts.load(), 1, ());
  renderer.Stop();
}

UNIT_TEST(FrameScheduling_DisableAndStopSuspendedSurface)
{
  df::ThreadsCommutator commutator;
  TestFactory factory;
  TestRenderer renderer(commutator, factory, dp::FrameStatus::Suspended);
  auto attempted = renderer.m_firstAttempt.get_future();
  renderer.Start();
  TEST(attempted.wait_for(5s) == std::future_status::ready, ());

  auto disabled = std::async(std::launch::async, [&renderer] { renderer.SetRenderingDisabled(false); });
  auto const status = disabled.wait_for(5s);
  // Stop also releases the rendering-enable wait if the disable handshake did not finish.
  renderer.Stop();
  TEST(status == std::future_status::ready, ());
  disabled.get();
}

UNIT_TEST(FrameScheduling_RecoveryRetriesWithoutMessages)
{
  for (auto const status : {dp::FrameStatus::Retry, dp::FrameStatus::RetryLater})
  {
    df::ThreadsCommutator commutator;
    TestFactory factory;
    TestRenderer renderer(commutator, factory, status);
    auto ready = renderer.m_frameReady.get_future();
    renderer.Start();
    TEST(ready.wait_for(5s) == std::future_status::ready, ());
    TEST_EQUAL(renderer.m_attempts.load(), 2, ());
  }
}

UNIT_TEST(FrameScheduling_ImmediateRetryPreservesCancellation)
{
  df::ThreadsCommutator commutator;
  TestFactory factory;
  TestRenderer renderer(commutator, factory, dp::FrameStatus::Retry);
  renderer.CancelMessageWaiting();
  renderer.ProcessFrameFailure(dp::FrameStatus::Retry);
  auto waiter =
      std::async(std::launch::async, [&renderer] { renderer.ProcessFrameFailure(dp::FrameStatus::Suspended); });
  auto const status = waiter.wait_for(5s);
  renderer.CancelMessageWaiting();
  waiter.get();
  TEST(status == std::future_status::ready, ());
}
}  // namespace frame_scheduling_tests

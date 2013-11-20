#include "backend_renderer.hpp"

#include "threads_commutator.hpp"
#include "update_coverage_message.hpp"
#include "resize_message.hpp"
#include "impl/backend_renderer_impl.hpp"

#include "../std/bind.hpp"

namespace df
{
  BackendRenderer::BackendRenderer(ThreadsCommutator * commutator,
                                   double visualScale,
                                   int surfaceWidth,
                                   int surfaceHeight)
  {
    m_impl = new BackendRendererImpl(commutator, visualScale, surfaceWidth, surfaceHeight);
    m_post = bind(&ThreadsCommutator::PostMessage, commutator, ThreadsCommutator::ResourceUploadThread, _1);
  }

  BackendRenderer::~BackendRenderer()
  {
    delete m_impl;
  }

  void BackendRenderer::UpdateCoverage(const ScreenBase & screen)
  {
    m_post(new UpdateCoverageMessage(screen));
  }

  void BackendRenderer::Resize(int x0, int y0, int w, int h)
  {
    m_post(new ResizeMessage(x0, y0, w, h));
  }
}

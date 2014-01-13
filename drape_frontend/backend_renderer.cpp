#include "backend_renderer.hpp"

#include "threads_commutator.hpp"
#include "message_subclasses.hpp"
#include "impl/backend_renderer_impl.hpp"

#include "../std/bind.hpp"

namespace df
{
  BackendRenderer::BackendRenderer(RefPointer<ThreadsCommutator> commutator,
                                   RefPointer<OGLContextFactory> oglcontextfactory,
                                   double visualScale,
                                   int surfaceWidth,
                                   int surfaceHeight)
    : m_commutator(commutator)
  {
    m_impl.Reset(new BackendRendererImpl(m_commutator, oglcontextfactory, visualScale, surfaceWidth, surfaceHeight));
  }

  BackendRenderer::~BackendRenderer()
  {
    m_impl.Destroy();
  }

  void BackendRenderer::UpdateCoverage(const ScreenBase & screen)
  {
    m_commutator->PostMessage(ThreadsCommutator::ResourceUploadThread, MovePointer<Message>(new UpdateCoverageMessage(screen)));
  }

  void BackendRenderer::Resize(int x0, int y0, int w, int h)
  {
    m_commutator->PostMessage(ThreadsCommutator::ResourceUploadThread, MovePointer<Message>(new ResizeMessage(x0, y0, w, h)));
  }
}

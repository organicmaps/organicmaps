#include "shape.hpp"

#include "../drape/utils/projection.hpp"

namespace gui
{

void Shape::SetPosition(gui::Position const & position)
{
  m_position = position;
}

bool Handle::IndexesRequired() const
{
  return false;
}

m2::RectD Handle::GetPixelRect(const ScreenBase & screen) const
{
  return m2::RectD();
}

void Handle::GetPixelShape(const ScreenBase & screen, dp::OverlayHandle::Rects & rects) const
{
  UNUSED_VALUE(screen);
  UNUSED_VALUE(rects);
}

ShapeRenderer::~ShapeRenderer()
{
  for (ShapeInfo & shape : m_shapes)
  {
    shape.m_handle.Destroy();
    shape.m_buffer.Destroy();
  }
}

dp::Batcher::TFlushFn ShapeRenderer::GetFlushRoutine()
{
  dp::Batcher::TFlushFn flushFn = [this](dp::GLState const & state, dp::TransferPointer<dp::RenderBucket> bucket)
  {
    dp::MasterPointer<dp::RenderBucket> b(bucket);
    ASSERT(b->GetOverlayHandlesCount() == 1, ());
    dp::TransferPointer<dp::VertexArrayBuffer> buffer = b->MoveBuffer();
    dp::TransferPointer<dp::OverlayHandle> transferH = b->PopOverlayHandle();
    b.Destroy();

    m_shapes.emplace_back(state, buffer, transferH);
  };

  return flushFn;
}

void ShapeRenderer::Build(dp::RefPointer<dp::GpuProgramManager> mng)
{
  for (ShapeInfo & shape : m_shapes)
    shape.m_buffer->Build(mng->GetProgram(shape.m_state.GetProgramIndex()));
}

void ShapeRenderer::Render(ScreenBase const & screen, dp::RefPointer<dp::GpuProgramManager> mng)
{
  array<float, 16> m;
  m2::RectD const & pxRect = screen.PixelRect();
  dp::MakeProjection(m, 0.0f, pxRect.SizeX(), pxRect.SizeY(), 0.0f);

  dp::UniformValuesStorage uniformStorage;
  uniformStorage.SetMatrix4x4Value("projection", m.data());

  for (ShapeInfo & shape : m_shapes)
  {
    Handle * handle = static_cast<Handle *>(shape.m_handle.GetRaw());
    handle->Update(screen);
    if (!(handle->IsValid() && handle->IsVisible()))
      return;

    dp::RefPointer<dp::GpuProgram> prg = mng->GetProgram(shape.m_state.GetProgramIndex());
    prg->Bind();
    dp::ApplyState(shape.m_state, prg);
    dp::ApplyUniforms(handle->GetUniforms(), prg);
    dp::ApplyUniforms(uniformStorage, prg);

    if (handle->HasDynamicAttributes())
    {
      dp::AttributeBufferMutator mutator;
      dp::RefPointer<dp::AttributeBufferMutator> mutatorRef = dp::MakeStackRefPointer(&mutator);
      handle->GetAttributeMutation(mutatorRef, screen);
      shape.m_buffer->ApplyMutation(dp::MakeStackRefPointer<dp::IndexBufferMutator>(nullptr), mutatorRef);
    }

    shape.m_buffer->Render();
  }
}

}

#include "shape.hpp"

#include "../drape/utils/projection.hpp"

namespace gui
{

void Shape::SetPosition(gui::Position const & position)
{
  m_position = position;
}

void Handle::SetProjection(int w, int h)
{
  array<float, 16> m;
  dp::MakeProjection(m, 0.0f, w, h, 0.0f);
  m_uniforms.SetMatrix4x4Value("projection", m.data());
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

ShapeRenderer::ShapeRenderer(dp::GLState const & state,
                             dp::TransferPointer<dp::VertexArrayBuffer> buffer,
                             dp::TransferPointer<dp::OverlayHandle> implHandle)
  : m_state(state)
  , m_buffer(buffer)
  , m_implHandle(implHandle)
{
}

ShapeRenderer::~ShapeRenderer()
{
  m_implHandle.Destroy();
  m_buffer.Destroy();
}

void ShapeRenderer::Build(dp::RefPointer<dp::GpuProgramManager> mng)
{
  m_buffer->Build(mng->GetProgram(m_state.GetProgramIndex()));
}

void ShapeRenderer::Render(ScreenBase const & screen, dp::RefPointer<dp::GpuProgramManager> mng)
{
  Handle * handle = static_cast<Handle *>(m_implHandle.GetRaw());
  handle->Update(screen);
  if (!(handle->IsValid() && handle->IsVisible()))
    return;

  dp::RefPointer<dp::GpuProgram> prg = mng->GetProgram(m_state.GetProgramIndex());
  prg->Bind();
  dp::ApplyState(m_state, prg);
  dp::ApplyUniforms(handle->GetUniforms(), prg);

  if (handle->HasDynamicAttributes())
  {
    dp::AttributeBufferMutator mutator;
    dp::RefPointer<dp::AttributeBufferMutator> mutatorRef = dp::MakeStackRefPointer(&mutator);
    handle->GetAttributeMutation(mutatorRef, screen);
    m_buffer->ApplyMutation(dp::MakeStackRefPointer<dp::IndexBufferMutator>(nullptr), mutatorRef);
  }

  m_buffer->Render();
}

}

#include "shape.hpp"

#include "../drape/glsl_func.hpp"
#include "../drape/utils/projection.hpp"

namespace gui
{
Handle::Handle(dp::Anchor anchor, const m2::PointF & pivot, const m2::PointF & size)
    : dp::OverlayHandle(FeatureID(), anchor, 0.0), m_pivot(glsl::ToVec2(pivot)), m_size(size)
{
}

void Handle::Update(const ScreenBase & screen)
{
  using namespace glsl;

  if (IsVisible())
    m_uniforms.SetMatrix4x4Value("modelView",
                                 value_ptr(transpose(translate(mat4(), vec3(m_pivot, 0.0)))));
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
  ForEachShapeInfo([](ShapeControl::ShapeInfo & info)
                   {
                     info.Destroy();
                   });
}

void ShapeRenderer::Build(dp::RefPointer<dp::GpuProgramManager> mng)
{
  ForEachShapeInfo([mng](ShapeControl::ShapeInfo & info) mutable
                   {
                     info.m_buffer->Build(mng->GetProgram(info.m_state.GetProgramIndex()));
                   });
}

void ShapeRenderer::Render(ScreenBase const & screen, dp::RefPointer<dp::GpuProgramManager> mng)
{
  array<float, 16> m;
  m2::RectD const & pxRect = screen.PixelRect();
  dp::MakeProjection(m, 0.0f, pxRect.SizeX(), pxRect.SizeY(), 0.0f);

  dp::UniformValuesStorage uniformStorage;
  uniformStorage.SetMatrix4x4Value("projection", m.data());

  ForEachShapeInfo(
      [&uniformStorage, &screen, mng](ShapeControl::ShapeInfo & info) mutable
      {
        Handle * handle = info.m_handle.GetRaw();
        handle->Update(screen);
        if (!(handle->IsValid() && handle->IsVisible()))
          return;

        dp::RefPointer<dp::GpuProgram> prg = mng->GetProgram(info.m_state.GetProgramIndex());
        prg->Bind();
        dp::ApplyState(info.m_state, prg);
        dp::ApplyUniforms(handle->GetUniforms(), prg);
        dp::ApplyUniforms(uniformStorage, prg);

        if (handle->HasDynamicAttributes())
        {
          dp::AttributeBufferMutator mutator;
          dp::RefPointer<dp::AttributeBufferMutator> mutatorRef = dp::MakeStackRefPointer(&mutator);
          handle->GetAttributeMutation(mutatorRef, screen);
          info.m_buffer->ApplyMutation(dp::MakeStackRefPointer<dp::IndexBufferMutator>(nullptr),
                                       mutatorRef);
        }

        info.m_buffer->Render();
      });
}

void ShapeRenderer::AddShape(dp::GLState const & state, dp::TransferPointer<dp::RenderBucket> bucket)
{
  m_shapes.push_back(ShapeControl());
  m_shapes.back().AddShape(state, bucket);
}

void ShapeRenderer::AddShapeControl(ShapeControl && control) { m_shapes.push_back(move(control)); }

void ShapeRenderer::ForEachShapeControl(TShapeControlEditFn const & fn)
{
  for_each(m_shapes.begin(), m_shapes.end(), fn);
}

void ShapeRenderer::ForEachShapeInfo(ShapeRenderer::TShapeInfoEditFn const & fn)
{
  ForEachShapeControl([&fn](ShapeControl & shape)
                      {
                        for_each(shape.m_shapesInfo.begin(), shape.m_shapesInfo.end(), fn);
                      });
}

ShapeControl::ShapeInfo::ShapeInfo(dp::GLState const & state,
                                   dp::TransferPointer<dp::VertexArrayBuffer> buffer,
                                   dp::TransferPointer<Handle> handle)
    : m_state(state), m_buffer(buffer), m_handle(handle)
{
}

void ShapeControl::ShapeInfo::Destroy()
{
  m_handle.Destroy();
  m_buffer.Destroy();
}

void ShapeControl::AddShape(dp::GLState const & state, dp::TransferPointer<dp::RenderBucket> bucket)
{
  dp::MasterPointer<dp::RenderBucket> b(bucket);
  ASSERT(b->GetOverlayHandlesCount() == 1, ());
  dp::TransferPointer<dp::VertexArrayBuffer> buffer = b->MoveBuffer();
  dp::TransferPointer<dp::OverlayHandle> transferH = b->PopOverlayHandle();
  dp::OverlayHandle * handle = dp::MasterPointer<dp::OverlayHandle>(transferH).Release();
  b.Destroy();

  ASSERT(dynamic_cast<Handle *>(handle) != nullptr, ());

  m_shapesInfo.push_back(ShapeInfo());
  ShapeInfo & info = m_shapesInfo.back();
  info.m_state = state;
  info.m_buffer = dp::MasterPointer<dp::VertexArrayBuffer>(buffer);
  info.m_handle = dp::MasterPointer<Handle>(static_cast<Handle *>(handle));
}

void ArrangeShapes(dp::RefPointer<ShapeRenderer> renderer, ShapeRenderer::TShapeControlEditFn const & fn)
{
  renderer->ForEachShapeControl(fn);
}

}

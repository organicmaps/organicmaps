#include "shape.hpp"

#include "drape/glsl_func.hpp"
#include "drape/utils/projection.hpp"

#include "base/logging.hpp"

namespace gui
{
Handle::Handle(dp::Anchor anchor, const m2::PointF & pivot, const m2::PointF & size)
    : dp::OverlayHandle(FeatureID(), anchor, 0.0), m_pivot(glsl::ToVec2(pivot)), m_size(size)
{
}

void Handle::Update(ScreenBase const & screen)
{
  using namespace glsl;

  if (IsVisible())
  {
    m_uniforms.SetMatrix4x4Value("modelView",
                                 value_ptr(transpose(translate(mat4(), vec3(m_pivot, 0.0)))));
    m_uniforms.SetFloatValue("u_opacity", 1.0);
  }
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

bool TappableHandle::IsTapped(m2::RectD const & touchArea) const
{
  if (!IsVisible() || !IsValid())
    return false;

  if (m_anchor == dp::Anchor::Center)
  {
    m2::RectD rect(m_pivot.x - m_size.x * 0.5, m_pivot.y - m_size.y * 0.5,
                   m_pivot.x + m_size.x * 0.5, m_pivot.y + m_size.y * 0.5);
    return rect.Intersect(touchArea);
  }
  else
  {
    LOG(LWARNING, ("Tapping on an overlay is not supported. Anchor type = ", m_anchor));
  }

  return false;
}

ShapeRenderer::~ShapeRenderer()
{
  ForEachShapeInfo([](ShapeControl::ShapeInfo & info)
                   {
                     info.Destroy();
                   });
}

void ShapeRenderer::Build(ref_ptr<dp::GpuProgramManager> mng)
{
  ForEachShapeInfo([mng](ShapeControl::ShapeInfo & info) mutable
                   {
                     info.m_buffer->Build(mng->GetProgram(info.m_state.GetProgramIndex()));
                   });
}

void ShapeRenderer::Render(ScreenBase const & screen, ref_ptr<dp::GpuProgramManager> mng)
{
  array<float, 16> m;
  m2::RectD const & pxRect = screen.PixelRect();
  dp::MakeProjection(m, 0.0f, pxRect.SizeX(), pxRect.SizeY(), 0.0f);

  dp::UniformValuesStorage uniformStorage;
  uniformStorage.SetMatrix4x4Value("projection", m.data());

  ForEachShapeInfo(
      [&uniformStorage, &screen, mng](ShapeControl::ShapeInfo & info) mutable
      {
        info.m_handle->Update(screen);
        if (!(info.m_handle->IsValid() && info.m_handle->IsVisible()))
          return;

        ref_ptr<dp::GpuProgram> prg = mng->GetProgram(info.m_state.GetProgramIndex());
        prg->Bind();
        dp::ApplyState(info.m_state, prg);
        dp::ApplyUniforms(info.m_handle->GetUniforms(), prg);
        dp::ApplyUniforms(uniformStorage, prg);

        if (info.m_handle->HasDynamicAttributes())
        {
          dp::AttributeBufferMutator mutator;
          ref_ptr<dp::AttributeBufferMutator> mutatorRef = make_ref(&mutator);
          info.m_handle->GetAttributeMutation(mutatorRef, screen);
          info.m_buffer->ApplyMutation(nullptr, mutatorRef);
        }

        info.m_buffer->Render();
      });
}

void ShapeRenderer::AddShape(dp::GLState const & state, drape_ptr<dp::RenderBucket> && bucket)
{
  m_shapes.push_back(ShapeControl());
  m_shapes.back().AddShape(state, move(bucket));
}

void ShapeRenderer::AddShapeControl(ShapeControl && control)
{
  m_shapes.push_back(move(control));
}

void ShapeRenderer::SetPivot(m2::PointF const & pivot)
{
  for (ShapeControl & control : m_shapes)
  {
    for (ShapeControl::ShapeInfo & info : control.m_shapesInfo)
      info.m_handle->SetPivot(glsl::ToVec2(pivot));
  }
}

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

ref_ptr<Handle> ShapeRenderer::ProcessTapEvent(m2::RectD const & touchArea)
{
  ref_ptr<Handle> resultHandle = nullptr;
  ForEachShapeInfo([&resultHandle, &touchArea](ShapeControl::ShapeInfo & shapeInfo)
                   {
                     if (shapeInfo.m_handle->IsTapped(touchArea))
                     {
                       ASSERT(resultHandle == nullptr, ("Overlays cannot be intersected"));
                       resultHandle = make_ref(shapeInfo.m_handle);
                     }
                   });

  return resultHandle;
}

ShapeControl::ShapeInfo::ShapeInfo(dp::GLState const & state,
                                   drape_ptr<dp::VertexArrayBuffer> && buffer,
                                   drape_ptr<Handle> && handle)
    : m_state(state), m_buffer(move(buffer)), m_handle(move(handle))
{
}

void ShapeControl::ShapeInfo::Destroy()
{
  m_handle.reset();
  m_buffer.reset();
}

void ShapeControl::AddShape(dp::GLState const & state, drape_ptr<dp::RenderBucket> && bucket)
{
  ASSERT(bucket->GetOverlayHandlesCount() == 1, ());

  drape_ptr<dp::OverlayHandle> handle = bucket->PopOverlayHandle();
  ASSERT(dynamic_cast<Handle *>(handle.get()) != nullptr, ());

  m_shapesInfo.push_back(ShapeInfo());
  ShapeInfo & info = m_shapesInfo.back();
  info.m_state = state;
  info.m_buffer = bucket->MoveBuffer();
  info.m_handle = drape_ptr<Handle>(static_cast<Handle*>(handle.release()));
}

void ArrangeShapes(ref_ptr<ShapeRenderer> renderer, ShapeRenderer::TShapeControlEditFn const & fn)
{
  renderer->ForEachShapeControl(fn);
}

}

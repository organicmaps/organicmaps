#pragma once

#include "drape_frontend/render_state_extension.hpp"

#include "shaders/program_manager.hpp"

#include "drape/graphics_context.hpp"
#include "drape/pointers.hpp"
#include "drape/vertex_array_buffer.hpp"

#include "geometry/point2d.hpp"

namespace df
{
class RenderNode
{
public:
  RenderNode(dp::RenderState const & state, drape_ptr<dp::VertexArrayBuffer> && buffer)
    : m_state(state)
    , m_buffer(std::move(buffer))
  {}

  template <typename ParamsType>
  void Render(ref_ptr<dp::GraphicsContext> context, ref_ptr<gpu::ProgramManager> mng, ParamsType const & params,
              dp::IndicesRange const & range)
  {
    Apply(context, mng, params);
    m_buffer->RenderRange(context, m_state.GetDrawAsLine(), range);
  }

  template <typename ParamsType>
  void Render(ref_ptr<dp::GraphicsContext> context, ref_ptr<gpu::ProgramManager> mng, ParamsType const & params)
  {
    Apply(context, mng, params);
    m_buffer->Render(context, m_state.GetDrawAsLine());
  }

  void SetPivot(m2::PointD const & pivot) { m_pivot = pivot; }
  m2::PointD const & GetPivot() const { return m_pivot; }

  void SetBoundingBox(m2::RectD const & bbox) { m_boundingBox = bbox; }
  m2::RectD const & GetBoundingBox() const { return m_boundingBox; }

private:
  template <typename ParamsType>
  void Apply(ref_ptr<dp::GraphicsContext> context, ref_ptr<gpu::ProgramManager> mng, ParamsType const & params)
  {
    auto prg = mng->GetProgram(m_state.GetProgram<gpu::Program>());
    prg->Bind();
    if (!m_isBuilt)
    {
      m_buffer->Build(context, prg);
      m_isBuilt = true;
    }

    dp::ApplyState(context, prg, m_state);
    mng->GetParamsSetter()->Apply(context, prg, params);
  }

  dp::RenderState m_state;
  drape_ptr<dp::VertexArrayBuffer> m_buffer;
  bool m_isBuilt = false;
  m2::PointD m_pivot;
  m2::RectD m_boundingBox;
};
}  // namespace df

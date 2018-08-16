#pragma once

#include "drape_frontend/render_state_extension.hpp"

#include "shaders/program_manager.hpp"

#include "drape/graphics_context.hpp"
#include "drape/vertex_array_buffer.hpp"
#include "drape/pointers.hpp"

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
    m_buffer->RenderRange(m_state.GetDrawAsLine(), range);
  }

  template <typename ParamsType>
  void Render(ref_ptr<dp::GraphicsContext> context, ref_ptr<gpu::ProgramManager> mng, ParamsType const & params)
  {
    Apply(context, mng, params);
    m_buffer->Render(m_state.GetDrawAsLine());
  }

private:
  template <typename ParamsType>
  void Apply(ref_ptr<dp::GraphicsContext> context, ref_ptr<gpu::ProgramManager> mng, ParamsType const & params)
  {
    auto prg = mng->GetProgram(m_state.GetProgram<gpu::Program>());
    prg->Bind();
    if (!m_isBuilt)
    {
      m_buffer->Build(prg);
      m_isBuilt = true;
    }

    dp::ApplyState(context, prg, m_state);
    mng->GetParamsSetter()->Apply(prg, params);
  }

  dp::RenderState m_state;
  drape_ptr<dp::VertexArrayBuffer> m_buffer;
  bool m_isBuilt = false;
};
}  // namespace df

#include "drape_frontend/render_node.hpp"

#include "shaders/program_manager.hpp"

#include "drape/vertex_array_buffer.hpp"

#include <utility>

namespace df
{
RenderNode::RenderNode(dp::GLState const & state, drape_ptr<dp::VertexArrayBuffer> && buffer)
  : m_state(state)
  , m_buffer(std::move(buffer))
{}

void RenderNode::Render(ref_ptr<gpu::ProgramManager> mng, dp::UniformValuesStorage const & uniforms,
                        dp::IndicesRange const & range)
{
  Apply(mng, uniforms);
  m_buffer->RenderRange(m_state.GetDrawAsLine(), range);
}

void RenderNode::Render(ref_ptr<gpu::ProgramManager> mng, dp::UniformValuesStorage const & uniforms)
{
  Apply(mng, uniforms);
  m_buffer->Render(m_state.GetDrawAsLine());
}

void RenderNode::Apply(ref_ptr<gpu::ProgramManager> mng, dp::UniformValuesStorage const & uniforms)
{
  auto prg = mng->GetProgram(m_state.GetProgram<gpu::Program>());
  prg->Bind();
  if (!m_isBuilt)
  {
    m_buffer->Build(prg);
    m_isBuilt = true;
  }

  dp::ApplyState(m_state, prg);
  dp::ApplyUniforms(uniforms, prg);
}
}  // namespace df

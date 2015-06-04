#include "render_node.hpp"

#include "drape/vertex_array_buffer.hpp"
#include "drape/gpu_program_manager.hpp"

namespace df
{

RenderNode::RenderNode(dp::GLState const & state, drape_ptr<dp::VertexArrayBuffer> && buffer)
  : m_state(state)
  , m_buffer(move(buffer))
  , m_isBuilded(false)
{
}

void RenderNode::Render(ref_ptr<dp::GpuProgramManager> mng, dp::UniformValuesStorage const & uniforms,
                        dp::IndicesRange const & range)
{
  Apply(mng, uniforms);
  m_buffer->RenderRange(range);
}

void RenderNode::Render(ref_ptr<dp::GpuProgramManager> mng, dp::UniformValuesStorage const & uniforms)
{
  Apply(mng, uniforms);
  m_buffer->Render();
}

void RenderNode::Apply(ref_ptr<dp::GpuProgramManager> mng, dp::UniformValuesStorage const & uniforms)
{
  ref_ptr<dp::GpuProgram> prg = mng->GetProgram(m_state.GetProgramIndex());
  prg->Bind();
  if (!m_isBuilded)
  {
    m_buffer->Build(prg);
    m_isBuilded = true;
  }

  dp::ApplyUniforms(uniforms, prg);
  dp::ApplyState(m_state, prg);
}

}

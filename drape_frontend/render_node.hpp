#pragma once

#include "drape/glstate.hpp"
#include "drape/pointers.hpp"

namespace dp
{
  class VertexArrayBuffer;
  class GpuProgramManager;
  struct IndicesRange;
}

namespace df
{

class RenderNode
{
public:
  RenderNode(dp::GLState const & state, drape_ptr<dp::VertexArrayBuffer> && buffer);

  void Render(ref_ptr<dp::GpuProgramManager> mng, dp::UniformValuesStorage const & uniforms);
  void Render(ref_ptr<dp::GpuProgramManager> mng, dp::UniformValuesStorage const & uniforms, dp::IndicesRange const & range);

private:
  void Apply(ref_ptr<dp::GpuProgramManager> mng, dp::UniformValuesStorage const & uniforms);

private:
  dp::GLState m_state;
  drape_ptr<dp::VertexArrayBuffer> m_buffer;
  bool m_isBuilded;
};

}

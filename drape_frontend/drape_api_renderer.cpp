#include "drape_frontend/drape_api_renderer.hpp"
#include "drape_frontend/shape_view_params.hpp"
#include "drape_frontend/visual_params.hpp"

#include "shaders/programs.hpp"

#include "drape/overlay_handle.hpp"
#include "drape/vertex_array_buffer.hpp"

#include <algorithm>

namespace df
{
void DrapeApiRenderer::AddRenderProperties(ref_ptr<gpu::ProgramManager> mng,
                                           std::vector<drape_ptr<DrapeApiRenderProperty>> && properties)
{
  if (properties.empty())
    return;

  size_t const startIndex = m_properties.size();
  m_properties.reserve(m_properties.size() + properties.size());
  move(properties.begin(), properties.end(), std::back_inserter(m_properties));
  for (size_t i = startIndex; i < m_properties.size(); i++)
  {
    for (auto const & bucket : m_properties[i]->m_buckets)
    {
      auto program = mng->GetProgram(bucket.first.GetProgram<gpu::Program>());
      program->Bind();
      bucket.second->GetBuffer()->Build(program);
    }
  }
}

void DrapeApiRenderer::RemoveRenderProperty(string const & id)
{
  m_properties.erase(std::remove_if(m_properties.begin(), m_properties.end(),
                                    [&id](auto const & p) { return p->m_id == id; }),
                                    m_properties.end());
}

void DrapeApiRenderer::Clear()
{
  m_properties.clear();
}

void DrapeApiRenderer::Render(ScreenBase const & screen, ref_ptr<gpu::ProgramManager> mng,
                              dp::UniformValuesStorage const & commonUniforms)
{
  if (m_properties.empty())
    return;

  auto const & params = df::VisualParams::Instance().GetGlyphVisualParams();
  for (auto const & property : m_properties)
  {
    math::Matrix<float, 4, 4> const mv = screen.GetModelView(property->m_center, kShapeCoordScalar);
    for (auto const & bucket : property->m_buckets)
    {
      auto program = mng->GetProgram(bucket.first.GetProgram<gpu::Program>());
      program->Bind();
      dp::ApplyState(bucket.first, program);

      dp::UniformValuesStorage uniforms = commonUniforms;
      uniforms.SetMatrix4x4Value("u_modelView", mv.m_data);
      uniforms.SetFloatValue("u_opacity", 1.0f);
      if (bucket.first.GetProgram<gpu::Program>() == gpu::Program::TextOutlinedGui)
      {
        uniforms.SetFloatValue("u_contrastGamma", params.m_guiContrast, params.m_guiGamma);
        uniforms.SetFloatValue("u_isOutlinePass", 0.0f);
      }
      dp::ApplyUniforms(uniforms, program);

      bucket.second->Render(bucket.first.GetDrawAsLine());
    }
  }
}
}  // namespace df

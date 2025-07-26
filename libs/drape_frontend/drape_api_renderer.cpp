#include "drape_frontend/drape_api_renderer.hpp"
#include "drape_frontend/shape_view_params.hpp"
#include "drape_frontend/visual_params.hpp"

#include "drape/overlay_handle.hpp"
#include "drape/vertex_array_buffer.hpp"

#include <algorithm>

namespace df
{
void DrapeApiRenderer::AddRenderProperties(ref_ptr<dp::GraphicsContext> context, ref_ptr<gpu::ProgramManager> mng,
                                           std::vector<drape_ptr<DrapeApiRenderProperty>> && properties)
{
  if (properties.empty())
    return;

  size_t const startIndex = m_properties.size();
  m_properties.reserve(m_properties.size() + properties.size());
  std::move(properties.begin(), properties.end(), std::back_inserter(m_properties));
  for (size_t i = startIndex; i < m_properties.size(); i++)
  {
    for (auto const & bucket : m_properties[i]->m_buckets)
    {
      auto program = mng->GetProgram(bucket.first.GetProgram<gpu::Program>());
      program->Bind();
      bucket.second->GetBuffer()->Build(context, program);
    }
  }
}

void DrapeApiRenderer::RemoveRenderProperty(std::string const & id)
{
  m_properties.erase(
      std::remove_if(m_properties.begin(), m_properties.end(), [&id](auto const & p) { return p->m_id == id; }),
      m_properties.end());
}

void DrapeApiRenderer::Clear()
{
  m_properties.clear();
}

void DrapeApiRenderer::Render(ref_ptr<dp::GraphicsContext> context, ref_ptr<gpu::ProgramManager> mng,
                              ScreenBase const & screen, FrameValues const & frameValues)
{
  if (m_properties.empty())
    return;

  auto const & glyphParams = df::VisualParams::Instance().GetGlyphVisualParams();
  for (auto const & property : m_properties)
  {
    math::Matrix<float, 4, 4> const mv = screen.GetModelView(property->m_center, kShapeCoordScalar);
    for (auto const & bucket : property->m_buckets)
    {
      auto program = mng->GetProgram(bucket.first.GetProgram<gpu::Program>());
      program->Bind();
      dp::ApplyState(context, program, bucket.first);

      auto const p = bucket.first.GetProgram<gpu::Program>();
      if (p == gpu::Program::TextOutlinedGui || p == gpu::Program::TextStaticOutlinedGui)
      {
        gpu::GuiProgramParams params;
        frameValues.SetTo(params);
        params.m_modelView = glsl::make_mat4(mv.m_data);
        params.m_contrastGamma = glsl::vec2(glyphParams.m_guiContrast, glyphParams.m_guiGamma);
        params.m_isOutlinePass = 0.0f;
        mng->GetParamsSetter()->Apply(context, program, params);
      }
      else
      {
        gpu::MapProgramParams params;
        frameValues.SetTo(params);
        params.m_modelView = glsl::make_mat4(mv.m_data);
        mng->GetParamsSetter()->Apply(context, program, params);
      }

      bucket.second->Render(context, bucket.first.GetDrawAsLine());
    }
  }
}
}  // namespace df

#pragma once

#include "drape_frontend/drape_api_builder.hpp"
#include "drape_frontend/frame_values.hpp"

#include "shaders/program_manager.hpp"

#include "geometry/screenbase.hpp"

#include <string>
#include <vector>

namespace df
{
class DrapeApiRenderer
{
public:
  DrapeApiRenderer() = default;

  void AddRenderProperties(ref_ptr<dp::GraphicsContext> context, ref_ptr<gpu::ProgramManager> mng,
                           std::vector<drape_ptr<DrapeApiRenderProperty>> && properties);
  void RemoveRenderProperty(std::string const & id);
  void Clear();

  void Render(ref_ptr<dp::GraphicsContext> context, ref_ptr<gpu::ProgramManager> mng, ScreenBase const & screen,
              FrameValues const & frameValues);

private:
  std::vector<drape_ptr<DrapeApiRenderProperty>> m_properties;
};
}  // namespace df

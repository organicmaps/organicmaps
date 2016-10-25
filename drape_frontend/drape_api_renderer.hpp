#pragma once

#include "drape_frontend/drape_api_builder.hpp"

#include "drape/gpu_program_manager.hpp"

#include "geometry/screenbase.hpp"

#include "std/vector.hpp"
#include "std/string.hpp"

namespace df
{

class DrapeApiRenderer
{
public:
  DrapeApiRenderer() = default;

  void AddRenderProperties(ref_ptr<dp::GpuProgramManager> mng,
                           vector<drape_ptr<DrapeApiRenderProperty>> && properties);

  void RemoveRenderProperty(string const & id);
  void Clear();

  void Render(ScreenBase const & screen, ref_ptr<dp::GpuProgramManager> mng,
              dp::UniformValuesStorage const & commonUniforms);

private:
  vector<drape_ptr<DrapeApiRenderProperty>> m_properties;
};

} // namespace df

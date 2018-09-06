#pragma once

#include "drape_frontend/render_state_extension.hpp"

#include "drape/color.hpp"
#include "drape/mesh_object.hpp"

#include "geometry/rect2d.hpp"

#include <vector>

namespace dp
{
class GpuProgram;
class TextureManager;
}  // namespace dp

namespace gpu
{
class ProgramManager;
}  // namespace gpu

class ScreenBase;

namespace df
{
class Arrow3d
{
  using Base = dp::MeshObject;
public:
  Arrow3d();

  void SetPosition(m2::PointD const & position);
  void SetAzimuth(double azimuth);
  void SetTexture(ref_ptr<dp::TextureManager> texMng);
  void SetPositionObsolete(bool obsolete);

  void Render(ref_ptr<dp::GraphicsContext> context, ref_ptr<gpu::ProgramManager> mng,
              ScreenBase const & screen, bool routingMode);

private:
  math::Matrix<float, 4, 4> CalculateTransform(ScreenBase const & screen, float dz,
                                               float scaleFactor) const;
  void RenderArrow(ref_ptr<dp::GraphicsContext> context, ref_ptr<gpu::ProgramManager> mng,
                   dp::MeshObject & mesh, ScreenBase const & screen, gpu::Program program,
                   dp::Color const & color, float dz, float scaleFactor);

  dp::MeshObject m_arrowMesh;
  dp::MeshObject m_shadowMesh;

  m2::PointD m_position;
  double m_azimuth = 0.0;
  bool m_obsoletePosition = false;

  dp::RenderState m_state;
};
}  // namespace df

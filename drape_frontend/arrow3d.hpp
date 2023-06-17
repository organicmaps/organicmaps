#pragma once

#include "drape_frontend/render_state_extension.hpp"

#include "drape/color.hpp"
#include "drape/glsl_types.hpp"
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
  explicit Arrow3d(ref_ptr<dp::GraphicsContext> context);

  static double GetMaxBottomSize();

  void SetPosition(m2::PointD const & position);
  void SetAzimuth(double azimuth);
  void SetTexture(ref_ptr<dp::TextureManager> texMng);
  void SetPositionObsolete(bool obsolete);
  
  // Leyout is axes (in the plane of map): x - right, y - up,
  // -z - perpendicular to the map's plane directed towards the observer.
  // Offset is in local coordinates (model's coordinates).
  void SetMeshOffset(glsl::vec3 const & offset);
  void SetMeshRotation(glsl::vec3 const & eulerAngles);
  void SetMeshScale(glsl::vec3 const & scale);

  void Render(ref_ptr<dp::GraphicsContext> context, ref_ptr<gpu::ProgramManager> mng,
              ScreenBase const & screen, bool routingMode);

private:
  glsl::mat4 CalculateTransform(ScreenBase const & screen, float dz,
                                float scaleFactor, dp::ApiVersion apiVersion) const;
  void RenderArrow(ref_ptr<dp::GraphicsContext> context, ref_ptr<gpu::ProgramManager> mng,
                   dp::MeshObject & mesh, ScreenBase const & screen, gpu::Program program,
                   dp::Color const & color, float dz, float scaleFactor);

  dp::MeshObject m_arrowMesh;
  dp::MeshObject m_shadowMesh;

  m2::PointD m_position;
  double m_azimuth = 0.0;
  bool m_obsoletePosition = false;

  dp::RenderState m_state;
  
  glsl::vec3 m_meshOffset{0.0f, 0.0f, 0.0f};
  glsl::vec3 m_meshEulerAngles{0.0f, 0.0f, 0.0f};
  glsl::vec3 m_meshScale{1.0f, 1.0f, 1.0f};
};
}  // namespace df

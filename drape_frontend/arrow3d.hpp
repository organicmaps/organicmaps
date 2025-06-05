#pragma once

#include "drape_frontend/drape_engine_params.hpp"
#include "drape_frontend/render_state_extension.hpp"

#include "drape/color.hpp"
#include "drape/glsl_types.hpp"
#include "drape/mesh_object.hpp"

#include "geometry/rect2d.hpp"

#include <optional>
#include <string>
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
  struct PreloadedMeshData
  {
    std::vector<float> m_positions;
    std::vector<float> m_normals;
    std::vector<float> m_texCoords;
  };

  struct PreloadedData
  {
    std::optional<PreloadedMeshData> m_meshData;
    std::optional<PreloadedMeshData> m_shadowMeshData;
    bool m_arrowMeshTexturingEnabled = false;
    glsl::vec2 m_texCoordFlipping{0.0f, 1.0f};

    glsl::vec3 m_meshOffset{0.0f, 0.0f, 0.0f};
    glsl::vec3 m_meshEulerAngles{0.0f, 0.0f, 0.0f};
    glsl::vec3 m_meshScale{1.0f, 1.0f, 1.0f};

    bool m_enableShadow = true;
    bool m_enableOutline = true;
  };

  static PreloadedData PreloadMesh(std::optional<Arrow3dCustomDecl> const & customDecl,
                                   ref_ptr<dp::TextureManager> texMng);

  Arrow3d(ref_ptr<dp::GraphicsContext> context, ref_ptr<dp::TextureManager> texMng, PreloadedData && preloadedData);

  bool IsValid() const;

  static double GetMaxBottomSize();

  void SetPosition(m2::PointD const & position);
  void SetAzimuth(double azimuth);
  void SetPositionObsolete(bool obsolete);

  // Leyout is axes (in the plane of map): x - right, y - up,
  // -z - perpendicular to the map's plane directed towards the observer.
  // Offset is in local coordinates (model's coordinates).
  void SetMeshOffset(glsl::vec3 const & offset);
  void SetMeshRotation(glsl::vec3 const & eulerAngles);
  void SetMeshScale(glsl::vec3 const & scale);

  void SetShadowEnabled(bool enabled);
  void SetOutlineEnabled(bool enabled);

  void Render(ref_ptr<dp::GraphicsContext> context, ref_ptr<gpu::ProgramManager> mng, ScreenBase const & screen,
              bool routingMode);

private:
  // Returns transform matrix and normal transform matrix.
  std::pair<glsl::mat4, glsl::mat4> CalculateTransform(ScreenBase const & screen, float dz, float scaleFactor,
                                                       dp::ApiVersion apiVersion) const;
  void RenderArrow(ref_ptr<dp::GraphicsContext> context, ref_ptr<gpu::ProgramManager> mng, dp::MeshObject & mesh,
                   ScreenBase const & screen, gpu::Program program, dp::Color const & color, float dz,
                   float scaleFactor);

  dp::MeshObject m_arrowMesh;
  bool const m_arrowMeshTexturingEnabled;
  glsl::vec2 const m_texCoordFlipping{0.0f, 1.0f};  // Y is flipped by default.

  drape_ptr<dp::MeshObject> m_shadowMesh;

  bool m_isValid = false;

  m2::PointD m_position;
  double m_azimuth = 0.0;
  bool m_obsoletePosition = false;

  dp::RenderState m_state;

  glsl::vec3 m_meshOffset{0.0f, 0.0f, 0.0f};
  glsl::vec3 m_meshEulerAngles{0.0f, 0.0f, 0.0f};
  glsl::vec3 m_meshScale{1.0f, 1.0f, 1.0f};

  bool m_enableShadow = true;
  bool m_enableOutline = true;
};
}  // namespace df

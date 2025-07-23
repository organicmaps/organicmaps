#pragma once

#include "drape/mesh_object.hpp"

#include "geometry/rect2d.hpp"

namespace gpu
{
class ProgramManager;
}  // namespace gpu

namespace df
{
class ScreenQuadRenderer : public dp::MeshObject
{
  using Base = dp::MeshObject;

public:
  explicit ScreenQuadRenderer(ref_ptr<dp::GraphicsContext> context);

  void SetTextureRect(ref_ptr<dp::GraphicsContext> context, m2::RectF const & rect);
  m2::RectF const & GetTextureRect() const { return m_textureRect; }

  // The parameter invertV is necessary, since there are some APIs (e.g. Metal) there render target
  // coordinates system is inverted by V axis.
  void RenderTexture(ref_ptr<dp::GraphicsContext> context, ref_ptr<gpu::ProgramManager> mng,
                     ref_ptr<dp::Texture> texture, float opacity, bool invertV = true);

private:
  void Rebuild(ref_ptr<dp::GraphicsContext> context);

  m2::RectF m_textureRect = m2::RectF(0.0f, 0.0f, 1.0f, 1.0f);
};
}  // namespace df

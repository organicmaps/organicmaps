#pragma once

#include "drape/mesh_object.hpp"

#include "geometry/rect2d.hpp"

namespace gpu
{
class ProgramManager;
}  // namespace gpu

namespace df
{
class ScreenQuadRenderer: public dp::MeshObject
{
  using Base = dp::MeshObject;
public:
  ScreenQuadRenderer();

  void SetTextureRect(m2::RectF const & rect);
  m2::RectF const & GetTextureRect() const { return m_textureRect; }

  void RenderTexture(ref_ptr<dp::GraphicsContext> context, ref_ptr<gpu::ProgramManager> mng,
                     ref_ptr<dp::Texture> texture, float opacity);

private:
  void Rebuild();

  m2::RectF m_textureRect = m2::RectF(0.0f, 0.0f, 1.0f, 1.0f);
};
}  // namespace df

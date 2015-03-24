#include "compass.hpp"

#include "../drape/glsl_types.hpp"
#include "../drape/glsl_func.hpp"
#include "../drape/shader_def.hpp"

#include "../drape/utils/vertex_decl.hpp"

#include "../std/bind.hpp"

namespace gui
{

namespace
{
  struct CompassVertex
  {
    CompassVertex(glsl::vec2 const & position, glsl::vec2 const & texCoord)
      : m_position(position)
      , m_texCoord(texCoord) {}

    glsl::vec2 m_position;
    glsl::vec2 m_texCoord;
  };

  class CompassHandle : public Handle
  {
  public:
    CompassHandle(m2::PointF const & pivot, m2::PointF const & size)
        : Handle(dp::Center, pivot, size)
    {
    }

    virtual void Update(ScreenBase const & screen) override
    {
      float angle = ang::AngleIn2PI(screen.GetAngle());
      if (angle < my::DegToRad(5.0) || angle > my::DegToRad(355.0))
        SetIsVisible(false);
      else
      {
        SetIsVisible(true);
        glsl::mat4 r = glsl::rotate(glsl::mat4(), angle, glsl::vec3(0.0, 0.0, 1.0));
        glsl::mat4 m = glsl::translate(glsl::mat4(), glsl::vec3(m_pivot, 0.0));
        m = glsl::transpose(m * r);
        m_uniforms.SetMatrix4x4Value("modelView", glsl::value_ptr(m));
      }
    }
  };
}

dp::TransferPointer<ShapeRenderer> Compass::Draw(dp::RefPointer<dp::TextureManager> tex) const
{
  dp::TextureManager::SymbolRegion region;
  tex->GetSymbolRegion("compass-image", region);
  glsl::vec2 halfSize = glsl::ToVec2(m2::PointD(region.GetPixelSize()) * 0.5);
  m2::RectF texRect = region.GetTexRect();

  ASSERT_EQUAL(m_position.m_anchor, dp::Center, ());
  CompassVertex vertexes[] =
  {
    CompassVertex(glsl::vec2(-halfSize.x, halfSize.y), glsl::ToVec2(texRect.LeftTop())),
    CompassVertex(glsl::vec2(-halfSize.x, -halfSize.y), glsl::ToVec2(texRect.LeftBottom())),
    CompassVertex(glsl::vec2(halfSize.x, halfSize.y), glsl::ToVec2(texRect.RightTop())),
    CompassVertex(glsl::vec2(halfSize.x, -halfSize.y), glsl::ToVec2(texRect.RightBottom()))
  };

  dp::GLState state(gpu::COMPASS_PROGRAM, dp::GLState::Gui);
  state.SetColorTexture(region.GetTexture());

  dp::AttributeProvider provider(1, 4);
  dp::BindingInfo info(2);

  dp::BindingDecl & posDecl = info.GetBindingDecl(0);
  posDecl.m_attributeName = "a_position";
  posDecl.m_componentCount = 2;
  posDecl.m_componentType = gl_const::GLFloatType;
  posDecl.m_offset = 0;
  posDecl.m_stride = sizeof(CompassVertex);

  dp::BindingDecl & texDecl = info.GetBindingDecl(1);
  texDecl.m_attributeName = "a_colorTexCoords";
  texDecl.m_componentCount = 2;
  texDecl.m_componentType = gl_const::GLFloatType;
  texDecl.m_offset = sizeof(glsl::vec2);
  texDecl.m_stride = posDecl.m_stride;

  provider.InitStream(0, info, dp::MakeStackRefPointer<void>(&vertexes));

  m2::PointF compassSize = region.GetPixelSize();
  dp::MasterPointer<dp::OverlayHandle> handle(
      new CompassHandle(m_position.m_pixelPivot, compassSize));

  dp::MasterPointer<ShapeRenderer> renderer(new ShapeRenderer());
  dp::Batcher batcher(dp::Batcher::IndexPerQuad, dp::Batcher::VertexPerQuad);
  dp::SessionGuard guard(batcher, bind(&ShapeRenderer::AddShape, renderer.GetRaw(), _1, _2));
  batcher.InsertTriangleStrip(state, dp::MakeStackRefPointer(&provider), handle.Move());

  return renderer.Move();
}

}

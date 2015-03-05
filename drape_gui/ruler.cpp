#include "ruler.hpp"
#include "ruler_helper.hpp"

#include "../drape/shader_def.hpp"

namespace gui
{

namespace
{

struct RulerVertex
{
  RulerVertex() = default;
  RulerVertex(glsl::vec2 const & pos, glsl::vec2 const & normal, glsl::vec2 const & texCoord)
    : m_position(pos)
    , m_normal(normal)
    , m_texCoord(texCoord)
  {
  }

  glsl::vec2 m_position;
  glsl::vec2 m_normal;
  glsl::vec2 m_texCoord;
};

dp::BindingInfo GetBindingInfo()
{
  dp::BindingInfo info(3);
  dp::BindingDecl & posDecl = info.GetBindingDecl(0);
  posDecl.m_attributeName = "a_position";
  posDecl.m_componentCount = 2;
  posDecl.m_componentType = gl_const::GLFloatType;
  posDecl.m_offset = 0;
  posDecl.m_stride = sizeof(RulerVertex);

  dp::BindingDecl & normalDecl = info.GetBindingDecl(1);
  normalDecl.m_attributeName = "a_normal";
  normalDecl.m_componentCount = 2;
  normalDecl.m_componentType = gl_const::GLFloatType;
  normalDecl.m_offset = sizeof(glsl::vec2);
  normalDecl.m_stride = posDecl.m_stride;

  dp::BindingDecl & texDecl = info.GetBindingDecl(2);
  texDecl.m_attributeName = "a_colorTexCoords";
  texDecl.m_componentCount = 2;
  texDecl.m_componentType = gl_const::GLFloatType;
  texDecl.m_offset = 2 * sizeof(glsl::vec2);
  texDecl.m_stride = posDecl.m_stride;

  return info;
}

class RulerHandle : public Handle
{
public:
  RulerHandle(dp::Anchor anchor)
    : Handle(anchor, m2::PointF(0.0f, 0.0f))
  {
    SetIsVisible(true);
  }

  virtual void Update(ScreenBase const & screen) override
  {
    RulerHelper & helper = RulerHelper::Instance();
    SetIsVisible(helper.IsVisible(screen));
    if (IsVisible())
      m_uniforms.SetFloatValue("u_length", helper.GetRulerPixelLength());
  }

  virtual bool IndexesRequired() const override { return false; }

  virtual m2::RectD GetPixelRect(ScreenBase const & screen) const override
  {
    return m2::RectD();
  }

  virtual void GetPixelShape(ScreenBase const & screen, Rects & rects) const override
  {
    UNUSED_VALUE(screen);
    UNUSED_VALUE(rects);
  }
};

}

void Ruler::Draw(dp::RefPointer<dp::Batcher> batcher, dp::RefPointer<dp::TextureManager> tex) const
{
  buffer_vector<RulerVertex, 4> data;

  dp::TextureManager::ColorRegion reg;
  tex->GetColorRegion(dp::Color(0x4D, 0x4D, 0x4D, 0xCC), reg);

  glsl::vec2 pivot = glsl::ToVec2(m_position.m_pixelPivot);
  glsl::vec2 texCoord = glsl::ToVec2(reg.GetTexRect().Center());
  float h = RulerHelper::Instance().GetRulerHalfHeight();

  glsl::vec2 normals[] =
  {
    glsl::vec2(-1.0, 0.0),
    glsl::vec2(1.0, 0.0),
  };

  dp::Anchor anchor = m_position.m_anchor;
  if (anchor & dp::Left)
    normals[0] = glsl::vec2(0.0, 0.0);
  else if (anchor & dp::Right)
    normals[1] = glsl::vec2(0.0, 0.0);

  data.push_back(RulerVertex(pivot + glsl::vec2(0.0, h), normals[0], texCoord));
  data.push_back(RulerVertex(pivot + glsl::vec2(0.0, -h), normals[0], texCoord));
  data.push_back(RulerVertex(pivot + glsl::vec2(0.0, h), normals[1], texCoord));
  data.push_back(RulerVertex(pivot + glsl::vec2(0.0, -h), normals[1], texCoord));

  dp::GLState state(gpu::RULER_PROGRAM, dp::GLState::UserMarkLayer);
  state.SetColorTexture(reg.GetTexture());

  dp::AttributeProvider provider(1, 4);
  provider.InitStream(0, GetBindingInfo(), dp::MakeStackRefPointer<void>(data.data()));
  batcher->InsertTriangleStrip(state, dp::MakeStackRefPointer(&provider),
                               dp::MovePointer<dp::OverlayHandle>(new RulerHandle(m_position.m_anchor)));
}

uint16_t Ruler::GetVertexCount() const
{
  return 4;
}

uint16_t Ruler::GetIndexCount() const
{
  return 6;
}

}

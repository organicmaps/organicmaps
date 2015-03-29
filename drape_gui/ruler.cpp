#include "drape_gui.hpp"
#include "gui_text.hpp"
#include "ruler.hpp"
#include "ruler_helper.hpp"

#include "../drape/glsl_func.hpp"
#include "../drape/glsl_types.hpp"
#include "../drape/shader_def.hpp"

#include "../std/bind.hpp"

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
  RulerHandle(dp::Anchor anchor, m2::PointF const & pivot)
      : Handle(anchor, pivot, m2::PointF::Zero())
  {
    SetIsVisible(true);
  }

  void Update(ScreenBase const & screen) override
  {
    RulerHelper & helper = DrapeGui::GetRulerHelper();
    SetIsVisible(helper.IsVisible(screen));
    if (IsVisible())
    {
      m_size = m2::PointF(helper.GetRulerPixelLength(), 2 * helper.GetRulerHalfHeight());
      m_uniforms.SetFloatValue("u_length", helper.GetRulerPixelLength());
    }
  }
};

class RulerTextHandle : public MutableLabelHandle
{
  typedef MutableLabelHandle TBase;

public:
  RulerTextHandle(dp::Anchor anchor, m2::PointF const & pivot)
    : MutableLabelHandle(anchor, pivot)
  {
  }

  void Update(ScreenBase const & screen) override
  {
    SetIsVisible(DrapeGui::GetRulerHelper().IsVisible(screen));
    if (IsVisible() && DrapeGui::GetRulerHelper().IsTextDirty())
      SetContent(DrapeGui::GetRulerHelper().GetRulerText());

    TBase::Update(screen);
  }
};

}

dp::TransferPointer<ShapeRenderer> Ruler::Draw(dp::RefPointer<dp::TextureManager> tex) const
{
  ShapeControl control;
  DrawRuler(control, tex);
  DrawText(control, tex);

  dp::MasterPointer<ShapeRenderer> renderer(new ShapeRenderer);
  renderer->AddShapeControl(move(control));
  return renderer.Move();
}

void Ruler::DrawRuler(ShapeControl & control, dp::RefPointer<dp::TextureManager> tex) const
{
  buffer_vector<RulerVertex, 4> data;

  dp::TextureManager::ColorRegion reg;
  tex->GetColorRegion(DrapeGui::GetGuiTextFont().m_color, reg);

  glsl::vec2 pivot = glsl::ToVec2(m_position.m_pixelPivot);
  glsl::vec2 texCoord = glsl::ToVec2(reg.GetTexRect().Center());
  float h = DrapeGui::GetRulerHelper().GetRulerHalfHeight();

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

  dp::GLState state(gpu::RULER_PROGRAM, dp::GLState::Gui);
  state.SetColorTexture(reg.GetTexture());

  dp::AttributeProvider provider(1, 4);
  provider.InitStream(0, GetBindingInfo(), dp::MakeStackRefPointer<void>(data.data()));

  {
    dp::Batcher batcher(dp::Batcher::IndexPerQuad, dp::Batcher::VertexPerQuad);
    dp::SessionGuard guard(batcher, bind(&ShapeControl::AddShape, &control, _1, _2));
    batcher.InsertTriangleStrip(state, dp::MakeStackRefPointer(&provider),
                                dp::MovePointer<dp::OverlayHandle>(
                                    new RulerHandle(m_position.m_anchor, m_position.m_pixelPivot)));
  }
}

void Ruler::DrawText(ShapeControl & control, dp::RefPointer<dp::TextureManager> tex) const
{
  string alphabet;
  size_t maxTextLength;
  RulerHelper & helper = DrapeGui::GetRulerHelper();
  helper.GetTextInitInfo(alphabet, maxTextLength);

  MutableLabelDrawer::Params params;
  params.m_anchor =
      static_cast<dp::Anchor>((m_position.m_anchor & (dp::Right | dp::Left)) | dp::Bottom);
  params.m_alphabet = alphabet;
  params.m_maxLength = maxTextLength;
  params.m_font = DrapeGui::GetGuiTextFont();
  params.m_pivot = m_position.m_pixelPivot + m2::PointF(0.0, helper.GetVerticalTextOffset());
  params.m_handleCreator = [](dp::Anchor anchor, m2::PointF const & pivot)
  {
    return dp::MovePointer<MutableLabelHandle>(new RulerTextHandle(anchor, pivot));
  };

  MutableLabelDrawer::Draw(params, tex, bind(&ShapeControl::AddShape, &control, _1, _2));
}

}

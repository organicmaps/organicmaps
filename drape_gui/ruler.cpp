#include "drape_gui.hpp"
#include "gui_text.hpp"
#include "ruler.hpp"
#include "ruler_helper.hpp"

#include "drape/glsl_func.hpp"
#include "drape/glsl_types.hpp"
#include "drape/shader_def.hpp"

#include "std/bind.hpp"

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
  uint8_t offset = 0;
  offset += dp::FillDecl<glsl::vec2, RulerVertex>(0, "a_position", info, offset);
  offset += dp::FillDecl<glsl::vec2, RulerVertex>(1, "a_normal", info, offset);
  offset += dp::FillDecl<glsl::vec2, RulerVertex>(2, "a_colorTexCoords", info, offset);

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
      m_uniforms.SetFloatValue("u_position", m_pivot.x, m_pivot.y);
    }
  }
};

class RulerTextHandle : public MutableLabelHandle
{
  typedef MutableLabelHandle TBase;

public:
  RulerTextHandle(dp::Anchor anchor, m2::PointF const & pivot)
    : MutableLabelHandle(anchor, pivot)
    , m_firstUpdate(true)
  {
  }

  void Update(ScreenBase const & screen) override
  {
    SetIsVisible(DrapeGui::GetRulerHelper().IsVisible(screen));
    if ((IsVisible() && DrapeGui::GetRulerHelper().IsTextDirty()) || m_firstUpdate)
    {
      SetContent(DrapeGui::GetRulerHelper().GetRulerText());
      m_firstUpdate = false;
    }

    TBase::Update(screen);
  }

  void SetPivot(glsl::vec2 const & pivot) override
  {
    RulerHelper & helper = DrapeGui::GetRulerHelper();
    TBase::SetPivot(pivot + glsl::vec2(0.0, helper.GetVerticalTextOffset() - helper.GetRulerHalfHeight()));
  }

private:
  bool m_firstUpdate;
};

}

drape_ptr<ShapeRenderer> Ruler::Draw(m2::PointF & size, ref_ptr<dp::TextureManager> tex) const
{
  ShapeControl control;
  size = m2::PointF::Zero();
  DrawRuler(size, control, tex);
  DrawText(size, control, tex);

  drape_ptr<ShapeRenderer> renderer = make_unique_dp<ShapeRenderer>();
  renderer->AddShapeControl(move(control));
  return renderer;
}

void Ruler::DrawRuler(m2::PointF & size, ShapeControl & control, ref_ptr<dp::TextureManager> tex) const
{
  buffer_vector<RulerVertex, 4> data;

  dp::TextureManager::ColorRegion reg;
  tex->GetColorRegion(DrapeGui::GetGuiTextFont().m_color, reg);

  glsl::vec2 texCoord = glsl::ToVec2(reg.GetTexRect().Center());
  float h = DrapeGui::GetRulerHelper().GetRulerHalfHeight();
  size += m2::PointF(DrapeGui::GetRulerHelper().GetMaxRulerPixelLength(), 2.0 * h);

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

  data.push_back(RulerVertex(glsl::vec2(0.0, h), normals[0], texCoord));
  data.push_back(RulerVertex(glsl::vec2(0.0, -h), normals[0], texCoord));
  data.push_back(RulerVertex(glsl::vec2(0.0, h), normals[1], texCoord));
  data.push_back(RulerVertex(glsl::vec2(0.0, -h), normals[1], texCoord));

  dp::GLState state(gpu::RULER_PROGRAM, dp::GLState::Gui);
  state.SetColorTexture(reg.GetTexture());

  dp::AttributeProvider provider(1, 4);
  provider.InitStream(0, GetBindingInfo(), make_ref(data.data()));

  {
    dp::Batcher batcher(dp::Batcher::IndexPerQuad, dp::Batcher::VertexPerQuad);
    dp::SessionGuard guard(batcher, bind(&ShapeControl::AddShape, &control, _1, _2));
    batcher.InsertTriangleStrip(state, make_ref(&provider),
                                make_unique_dp<RulerHandle>(m_position.m_anchor, m_position.m_pixelPivot));
  }
}

void Ruler::DrawText(m2::PointF & size, ShapeControl & control, ref_ptr<dp::TextureManager> tex) const
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
    return make_unique_dp<RulerTextHandle>(anchor, pivot);
  };

  m2::PointF textSize = MutableLabelDrawer::Draw(params, tex, bind(&ShapeControl::AddShape, &control, _1, _2));
  size.y += (textSize.y + abs(helper.GetVerticalTextOffset()));
}

}

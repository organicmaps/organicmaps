#include "drape_frontend/gui/ruler.hpp"

#include "drape_frontend/animation/show_hide_animation.hpp"
#include "drape_frontend/animation_constants.hpp"
#include "drape_frontend/batcher_bucket.hpp"
#include "drape_frontend/gui/drape_gui.hpp"
#include "drape_frontend/gui/gui_text.hpp"
#include "drape_frontend/gui/ruler_helper.hpp"

#include "shaders/programs.hpp"

#include "drape/glsl_types.hpp"

#include <functional>

using namespace std::placeholders;

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
  {}

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
  /*offset += */ dp::FillDecl<glsl::vec2, RulerVertex>(2, "a_colorTexCoords", info, offset);

  return info;
}

template <typename TBase>
class BaseRulerHandle : public TBase
{
public:
  BaseRulerHandle(uint32_t id, dp::Anchor anchor, m2::PointF const & pivot)
    : TBase(id, anchor, pivot)
    , m_animation(false, df::kOverlayAnimationTimeSec)
  {}

  bool Update(ScreenBase const & screen) override
  {
    m_animation.AnimateVisibility(RulerHelper::IsVisible(screen));
    TBase::SetIsVisible(m_animation.IsVisible());
    if (!TBase::IsVisible() || !TBase::Update(screen))
      return false;

    UpdateImpl(DrapeGui::GetRulerHelper());
    TBase::m_params.m_opacity = static_cast<float>(m_animation.GetVisibility());
    return true;
  }

protected:
  virtual void UpdateImpl(RulerHelper const & /*helper*/) {}

private:
  df::ShowHideAnimation m_animation;
};

class RulerHandle : public BaseRulerHandle<Handle>
{
  using TBase = BaseRulerHandle<Handle>;

public:
  RulerHandle(uint32_t id, dp::Anchor anchor, m2::PointF const & pivot) : TBase(id, anchor, pivot) {}

private:
  void UpdateImpl(RulerHelper const & helper) override
  {
    m_params.m_length = helper.GetRulerPixelLength();
    m_params.m_position = m_pivot;
  }
};

class RulerTextHandle : public BaseRulerHandle<MutableLabelHandle>
{
  using TBase = BaseRulerHandle<MutableLabelHandle>;

public:
  RulerTextHandle(uint32_t id, dp::Anchor anchor, m2::PointF const & pivot, ref_ptr<dp::TextureManager> textures)
    : TBase(id, anchor, pivot)
  {
    SetTextureManager(textures);
  }

  bool Update(ScreenBase const & screen) override
  {
    if (RulerHelper::IsVisible(screen) && (DrapeGui::GetRulerHelper().IsTextDirty() || m_firstUpdate))
    {
      SetContent(DrapeGui::GetRulerHelper().GetRulerText());
      m_firstUpdate = false;
    }

    return TBase::Update(screen);
  }

  void SetPivot(glsl::vec2 const & pivot) override
  {
    TBase::SetPivot(pivot + glsl::vec2(0.0, RulerHelper::GetVerticalTextOffset() - RulerHelper::GetRulerHalfHeight()));
  }

private:
  bool m_firstUpdate = true;
};
}  // namespace

drape_ptr<ShapeRenderer> Ruler::Draw(ref_ptr<dp::GraphicsContext> context, ref_ptr<dp::TextureManager> tex) const
{
  ShapeControl control;
  DrawRuler(context, control, tex);
  DrawText(context, control, tex);

  drape_ptr<ShapeRenderer> renderer = make_unique_dp<ShapeRenderer>();
  renderer->AddShapeControl(std::move(control));
  return renderer;
}

void Ruler::DrawRuler(ref_ptr<dp::GraphicsContext> context, ShapeControl & control,
                      ref_ptr<dp::TextureManager> tex) const
{
  buffer_vector<RulerVertex, 4> data;

  dp::TextureManager::ColorRegion reg;
  tex->GetColorRegion(DrapeGui::GetGuiTextFont().m_color, reg);

  glsl::vec2 texCoord = glsl::ToVec2(reg.GetTexRect().Center());
  float const h = RulerHelper::GetRulerHalfHeight();

  glsl::vec2 normals[] = {
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

  auto state = df::CreateRenderState(gpu::Program::Ruler, df::DepthLayer::GuiLayer);
  state.SetColorTexture(reg.GetTexture());
  state.SetDepthTestEnabled(false);

  dp::AttributeProvider provider(1, 4);
  provider.InitStream(0, GetBindingInfo(), make_ref(data.data()));

  {
    dp::Batcher batcher(dp::Batcher::IndexPerQuad, dp::Batcher::VertexPerQuad);
    batcher.SetBatcherHash(static_cast<uint64_t>(df::BatcherBucket::Default));
    dp::SessionGuard guard(context, batcher, std::bind(&ShapeControl::AddShape, &control, _1, _2));
    batcher.InsertTriangleStrip(
        context, state, make_ref(&provider),
        make_unique_dp<RulerHandle>(EGuiHandle::GuiHandleRuler, m_position.m_anchor, m_position.m_pixelPivot));
  }
}

void Ruler::DrawText(ref_ptr<dp::GraphicsContext> context, ShapeControl & control,
                     ref_ptr<dp::TextureManager> tex) const
{
  std::string alphabet;
  uint32_t maxTextLength;
  RulerHelper::GetTextInitInfo(alphabet, maxTextLength);

  MutableLabelDrawer::Params params;
  params.m_anchor = static_cast<dp::Anchor>((m_position.m_anchor & (dp::Right | dp::Left)) | dp::Bottom);
  params.m_alphabet = alphabet;
  params.m_maxLength = maxTextLength;
  params.m_font = DrapeGui::GetGuiTextFont();
  params.m_pivot = m_position.m_pixelPivot + m2::PointF(0.0f, RulerHelper::GetVerticalTextOffset());
  params.m_handleCreator = [tex](dp::Anchor anchor, m2::PointF const & pivot)
  { return make_unique_dp<RulerTextHandle>(EGuiHandle::GuiHandleRulerLabel, anchor, pivot, tex); };

  MutableLabelDrawer::Draw(context, params, tex, std::bind(&ShapeControl::AddShape, &control, _1, _2));
}
}  // namespace gui

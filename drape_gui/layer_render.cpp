#include "compass.hpp"
#include "country_status.hpp"
#include "drape_gui.hpp"
#include "gui_text.hpp"
#include "layer_render.hpp"
#include "ruler.hpp"
#include "ruler_helper.hpp"

#include "../drape/batcher.hpp"
#include "../drape/render_bucket.hpp"

#include "../base/stl_add.hpp"

#include "../std/bind.hpp"

namespace gui
{

namespace
{

class ScaleLabelHandle : public MutableLabelHandle
{
  using TBase = MutableLabelHandle;
public:
  ScaleLabelHandle()
    : TBase(dp::LeftBottom, m2::PointF::Zero())
    , m_scale(0)
  {
    SetIsVisible(true);
  }

  void Update(ScreenBase const & screen) override
  {
    int newScale = DrapeGui::Instance().GetGeneralization(screen);
    if (m_scale != newScale)
    {
      m_scale = newScale;
      SetContent("Scale : " + strings::to_string(m_scale));
    }

    float vs = DrapeGui::Instance().GetScaleFactor();
    m2::PointF offset(10.0f * vs, 30.0f * vs);

    SetPivot(glsl::ToVec2(m2::PointF(screen.PixelRect().LeftBottom()) + offset));
    TBase::Update(screen);
  }

private:
  int m_scale;
};

}

LayerCacher::LayerCacher(string const & deviceType)
{
  m_skin.reset(new Skin(ResolveGuiSkinFile(deviceType)));
}

void LayerCacher::Resize(int w, int h)
{
  m_skin->Resize(w, h);
}

dp::TransferPointer<LayerRenderer> LayerCacher::Recache(Skin::ElementName names,
                                                        dp::RefPointer<dp::TextureManager> textures)
{
  dp::MasterPointer<LayerRenderer> renderer(new LayerRenderer());

  if (names & Skin::Compass)
    renderer->AddShapeRenderer(Skin::Compass,
                               Compass(m_skin->ResolvePosition(Skin::Compass)).Draw(textures));
  if (names & Skin::Ruler)
    renderer->AddShapeRenderer(Skin::Ruler,
                               Ruler(m_skin->ResolvePosition(Skin::Ruler)).Draw(textures));
  if (names & Skin::CountryStatus)
    renderer->AddShapeRenderer(
        Skin::CountryStatus,
        CountryStatus(m_skin->ResolvePosition(Skin::CountryStatus)).Draw(textures));
  if (names & Skin::Copyright)
  {
    // TODO UVR
  }

#ifdef DEBUG
  MutableLabelDrawer::Params params;
  params.m_alphabet = "Scale: 1234567890";
  params.m_maxLength = 10;
  params.m_anchor = dp::LeftBottom;
  params.m_font = dp::FontDecl(dp::Color::Black(), 14);
  params.m_pivot = m2::PointF::Zero();
  params.m_handleCreator = [](dp::Anchor, m2::PointF const &)
  {
    return dp::MovePointer<MutableLabelHandle>(new ScaleLabelHandle());
  };

  dp::MasterPointer<ShapeRenderer> scaleRenderer(new ShapeRenderer());
  MutableLabelDrawer::Draw(params, textures, bind(&ShapeRenderer::AddShape, scaleRenderer.GetRaw(), _1, _2));

  renderer->AddShapeRenderer(Skin::ScaleLabel, scaleRenderer.Move());
#endif

  return renderer.Move();
}

//////////////////////////////////////////////////////////////////////////////////////////

LayerRenderer::~LayerRenderer()
{
  DestroyRenderers();
}

void LayerRenderer::Build(dp::RefPointer<dp::GpuProgramManager> mng)
{
  for (TRenderers::value_type & r : m_renderers)
    r.second->Build(mng);
}

void LayerRenderer::Render(dp::RefPointer<dp::GpuProgramManager> mng, ScreenBase const & screen)
{
  DrapeGui::Instance().GetRulerHelper().Update(screen);
  for (TRenderers::value_type & r : m_renderers)
    r.second->Render(screen, mng);
}

void LayerRenderer::Merge(dp::RefPointer<LayerRenderer> other)
{
  for (TRenderers::value_type & r : other->m_renderers)
  {
    dp::MasterPointer<ShapeRenderer> & guiElement = m_renderers[r.first];
    if (!guiElement.IsNull())
      guiElement.Destroy();
    guiElement = r.second;
  }

  other->m_renderers.clear();
}

void LayerRenderer::DestroyRenderers()
{
  DeleteRange(m_renderers, dp::MasterPointerDeleter());
}

void LayerRenderer::AddShapeRenderer(Skin::ElementName name,
                                     dp::TransferPointer<ShapeRenderer> shape)
{
  if (shape.IsNull())
    return;

  VERIFY(m_renderers.insert(make_pair(name, dp::MasterPointer<ShapeRenderer>(shape))).second, ());
}

}

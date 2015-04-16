#include "compass.hpp"
#include "copyright_label.hpp"
#include "country_status.hpp"
#include "drape_gui.hpp"
#include "gui_text.hpp"
#include "layer_render.hpp"
#include "ruler.hpp"
#include "ruler_helper.hpp"

#include "drape/batcher.hpp"
#include "drape/render_bucket.hpp"

#include "base/stl_add.hpp"

#include "std/bind.hpp"

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

drape_ptr<LayerRenderer> LayerCacher::Recache(Skin::ElementName names,
                                              ref_ptr<dp::TextureManager> textures)
{
  drape_ptr<LayerRenderer> renderer = make_unique_dp<LayerRenderer>();

  if (names & Skin::Compass)
  {
    renderer->AddShapeRenderer(Skin::Compass,
                               Compass(GetPos(Skin::Compass)).Draw(textures));
  }

  if (names & Skin::Ruler)
  {
    renderer->AddShapeRenderer(Skin::Ruler,
                               Ruler(GetPos(Skin::Ruler)).Draw(textures));
  }

  if (names & Skin::CountryStatus)
  {
    renderer->AddShapeRenderer(Skin::CountryStatus,
                               CountryStatus(GetPos(Skin::CountryStatus)).Draw(textures));
  }

  if (DrapeGui::Instance().IsCopyrightActive() && (names & Skin::Copyright))
  {
    renderer->AddShapeRenderer(Skin::Copyright,
                               CopyrightLabel(GetPos(Skin::Copyright)).Draw(textures));
  }


#ifdef DEBUG
  MutableLabelDrawer::Params params;
  params.m_alphabet = "Scale: 1234567890";
  params.m_maxLength = 10;
  params.m_anchor = dp::LeftBottom;
  params.m_font = DrapeGui::GetGuiTextFont();
  params.m_pivot = m2::PointF::Zero();
  params.m_handleCreator = [](dp::Anchor, m2::PointF const &)
  {
    return make_unique_dp<ScaleLabelHandle>();
  };

  drape_ptr<ShapeRenderer> scaleRenderer = make_unique_dp<ShapeRenderer>();
  MutableLabelDrawer::Draw(params, textures, bind(&ShapeRenderer::AddShape,
                                                  static_cast<ShapeRenderer*>(make_ref(scaleRenderer)), _1, _2));

  renderer->AddShapeRenderer(Skin::ScaleLabel, move(scaleRenderer));
#endif

  return renderer;
}

Position LayerCacher::GetPos(Skin::ElementName name)
{
  return m_skin->ResolvePosition(name);
}

//////////////////////////////////////////////////////////////////////////////////////////

LayerRenderer::~LayerRenderer()
{
  DestroyRenderers();
}

void LayerRenderer::Build(ref_ptr<dp::GpuProgramManager> mng)
{
  for (TRenderers::value_type & r : m_renderers)
    r.second->Build(mng);
}

void LayerRenderer::Render(ref_ptr<dp::GpuProgramManager> mng, ScreenBase const & screen)
{
  DrapeGui::Instance().GetRulerHelper().Update(screen);
  for (TRenderers::value_type & r : m_renderers)
    r.second->Render(screen, mng);
}

void LayerRenderer::Merge(ref_ptr<LayerRenderer> other)
{
  for (TRenderers::value_type & r : other->m_renderers)
  {
    TRenderers::iterator it = m_renderers.find(r.first);
    if (it != m_renderers.end())
    {
      it->second.reset();
      it->second = move(r.second);
    }
    else
    {
      m_renderers.insert(make_pair(r.first, move(r.second)));
    }
  }

  other->m_renderers.clear();
}

void LayerRenderer::DestroyRenderers()
{
  m_renderers.clear();
}

void LayerRenderer::AddShapeRenderer(Skin::ElementName name, drape_ptr<ShapeRenderer> && shape)
{
  if (shape == nullptr)
    return;

  VERIFY(m_renderers.insert(make_pair(name, move(shape))).second, ());
}

}

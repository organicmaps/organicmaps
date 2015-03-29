#include "compass.hpp"
#include "country_status.hpp"
#include "drape_gui.hpp"
#include "layer_render.hpp"
#include "ruler.hpp"
#include "ruler_helper.hpp"

#include "../drape/batcher.hpp"
#include "../drape/render_bucket.hpp"

#include "../base/stl_add.hpp"

#include "../std/bind.hpp"

namespace gui
{

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

#include "layer_render.hpp"
#include "compass.hpp"
#include "ruler.hpp"
#include "ruler_helper.hpp"

#include "../drape/batcher.hpp"
#include "../drape/render_bucket.hpp"

#include "../base/stl_add.hpp"

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

dp::TransferPointer<LayerRenderer> LayerCacher::Recache(dp::RefPointer<dp::TextureManager> textures)
{
  LayerRenderer * renderer = new LayerRenderer();

  renderer->AddShapeRenderer(CacheShape(textures, Compass(), Skin::ElementName::Compass));
  renderer->AddShapeRenderer(CacheShape(textures, Ruler(), Skin::ElementName::Ruler));

  return dp::MovePointer(renderer);
}

dp::TransferPointer<ShapeRenderer> LayerCacher::CacheShape(dp::RefPointer<dp::TextureManager> mng, Shape && shape, Skin::ElementName element)
{
  shape.SetPosition(m_skin->ResolvePosition(element));
  return shape.Draw(mng);
}

//////////////////////////////////////////////////////////////////////////////////////////

LayerRenderer::~LayerRenderer()
{
  DestroyRenderers();
}

void LayerRenderer::Build(dp::RefPointer<dp::GpuProgramManager> mng)
{
  for_each(m_renderers.begin(), m_renderers.end(), [mng](dp::MasterPointer<ShapeRenderer> r)
  {
    r->Build(mng);
  });
}

void LayerRenderer::Render(dp::RefPointer<dp::GpuProgramManager> mng, ScreenBase const & screen)
{
  RulerHelper::Instance().Update(screen);
  for_each(m_renderers.begin(), m_renderers.end(), [&screen, mng](dp::MasterPointer<ShapeRenderer> r)
  {
    r->Render(screen, mng);
  });
}

void LayerRenderer::DestroyRenderers()
{
  DeleteRange(m_renderers, dp::MasterPointerDeleter());
}

void LayerRenderer::AddShapeRenderer(dp::TransferPointer<ShapeRenderer> shape)
{
  m_renderers.emplace_back(shape);
}

}

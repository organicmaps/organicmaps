#include "layer_render.hpp"
#include "compass.hpp"
#include "ruler.hpp"
#include "ruler_text.hpp"
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
  dp::Batcher::TFlushFn flushFn = [&renderer, this](dp::GLState const & state, dp::TransferPointer<dp::RenderBucket> bucket)
  {
    dp::MasterPointer<dp::RenderBucket> b(bucket);
    ASSERT(b->GetOverlayHandlesCount() == 1, ());
    dp::TransferPointer<dp::VertexArrayBuffer> buffer = b->MoveBuffer();
    dp::TransferPointer<dp::OverlayHandle> transferH = b->PopOverlayHandle();
    dp::MasterPointer<dp::OverlayHandle> handle(transferH);
    b.Destroy();

    static_cast<Handle *>(handle.GetRaw())->SetProjection(m_skin->GetWidth(), m_skin->GetHeight());

    renderer->AddShapeRenderer(new ShapeRenderer(state, buffer, handle.Move()));
  };

  dp::Batcher batcher;
  dp::RefPointer<dp::Batcher> pBatcher = dp::MakeStackRefPointer(&batcher);

  CacheShape(flushFn, pBatcher, textures, Compass(), Skin::ElementName::Compass);
  CacheShape(flushFn, pBatcher, textures, Ruler(), Skin::ElementName::Ruler);
  CacheShape(flushFn, pBatcher, textures, RulerText(), Skin::ElementName::Ruler);

  return dp::MovePointer(renderer);
}

void LayerCacher::CacheShape(dp::Batcher::TFlushFn const & flushFn, dp::RefPointer<dp::Batcher> batcher,
                             dp::RefPointer<dp::TextureManager> mng, Shape && shape, Skin::ElementName element)
{
  shape.SetPosition(m_skin->ResolvePosition(element));

  batcher->SetVertexBufferSize(shape.GetVertexCount());
  batcher->SetIndexBufferSize(shape.GetIndexCount());
  batcher->StartSession(flushFn);
  shape.Draw(batcher, mng);
  batcher->EndSession();
}

//////////////////////////////////////////////////////////////////////////////////////////

LayerRenderer::~LayerRenderer()
{
  DestroyRenderers();
}

void LayerRenderer::Build(dp::RefPointer<dp::GpuProgramManager> mng)
{
  for_each(m_renderers.begin(), m_renderers.end(), [mng](ShapeRenderer * r)
  {
    r->Build(mng);
  });
}

void LayerRenderer::Render(dp::RefPointer<dp::GpuProgramManager> mng, ScreenBase const & screen)
{
  RulerHelper::Instance().Update(screen);
  for_each(m_renderers.begin(), m_renderers.end(), [&screen, mng](ShapeRenderer * r)
  {
    r->Render(screen, mng);
  });
}

void LayerRenderer::DestroyRenderers()
{
  DeleteRange(m_renderers, DeleteFunctor());
}

void LayerRenderer::AddShapeRenderer(ShapeRenderer * shape)
{
  m_renderers.push_back(shape);
}

}

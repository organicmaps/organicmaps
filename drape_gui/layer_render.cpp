#include "layer_render.hpp"
#include "compass.hpp"

#include "../drape/batcher.hpp"
#include "../drape/render_bucket.hpp"

#include "../base/stl_add.hpp"

namespace gui
{

LayerCacher::LayerCacher(string const & deviceType, double vs)
{
  m_skin.reset(new Skin(ResolveGuiSkinFile(deviceType), vs));
}

void LayerCacher::Resize(int w, int h)
{
  m_skin->Resize(w, h);
}

dp::TransferPointer<LayerRenderer> LayerCacher::Recache(dp::RefPointer<dp::TextureManager> textures)
{
  LayerRenderer * renderer = new LayerRenderer();

  Compass compass;
  compass.SetPosition(m_skin->ResolvePosition(Skin::ElementName::Compass));

  auto flushFn = [&renderer, this](dp::GLState const & state, dp::TransferPointer<dp::RenderBucket> bucket)
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

  dp::Batcher batcher(compass.GetIndexCount(), compass.GetVertexCount());
  batcher.StartSession(flushFn);
  compass.Draw(dp::MakeStackRefPointer(&batcher), textures);
  batcher.EndSession();

  return dp::MovePointer(renderer);
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

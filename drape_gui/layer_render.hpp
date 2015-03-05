#pragma once

#include "skin.hpp"
#include "shape.hpp"

#include "../drape/texture_manager.hpp"
#include "../drape/gpu_program_manager.hpp"

#include "../geometry/screenbase.hpp"

#include "../std/unique_ptr.hpp"

namespace gui
{

class LayerRenderer
{
public:
  LayerRenderer() = default;
  LayerRenderer(LayerRenderer && other) = delete;
  LayerRenderer(LayerRenderer const & other) = delete;

  LayerRenderer & operator=(LayerRenderer && other) = delete;
  LayerRenderer & operator=(LayerRenderer const & other) = delete;

  ~LayerRenderer();

  void Build(dp::RefPointer<dp::GpuProgramManager> mng);
  void Render(dp::RefPointer<dp::GpuProgramManager> mng, ScreenBase const & screen);

private:
  void DestroyRenderers();

  friend class LayerCacher;
  void AddShapeRenderer(ShapeRenderer * shape);

private:
  vector<ShapeRenderer *> m_renderers;
};

class LayerCacher
{
public:
  LayerCacher(string const & deviceType);

  void Resize(int w, int h);
  dp::TransferPointer<LayerRenderer> Recache(dp::RefPointer<dp::TextureManager> textures);

private:
  void CacheShape(dp::Batcher::TFlushFn const & flushFn, dp::RefPointer<dp::Batcher> batcher,
                  dp::RefPointer<dp::TextureManager> mng, Shape && shape, Skin::ElementName element);

private:
  unique_ptr<Skin> m_skin;
};

}

#pragma once

#include "skin.hpp"
#include "shape.hpp"

#include "../drape/texture_manager.hpp"
#include "../drape/gpu_program_manager.hpp"

#include "../geometry/screenbase.hpp"

#include "../std/map.hpp"
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
  void Merge(dp::RefPointer<LayerRenderer> other);

private:
  void DestroyRenderers();

  friend class LayerCacher;
  void AddShapeRenderer(Skin::ElementName name, dp::TransferPointer<ShapeRenderer> shape);

private:
  typedef map<Skin::ElementName, dp::MasterPointer<ShapeRenderer> > TRenderers;
  TRenderers m_renderers;
};

class LayerCacher
{
public:
  LayerCacher(string const & deviceType);

  void Resize(int w, int h);
  /// @param names - can be combinations of single flags, or equal AllElements
  dp::TransferPointer<LayerRenderer> Recache(Skin::ElementName names, dp::RefPointer<dp::TextureManager> textures);

private:
  unique_ptr<Skin> m_skin;
};

}

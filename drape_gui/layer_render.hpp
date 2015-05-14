#pragma once

#include "skin.hpp"
#include "shape.hpp"

#include "drape/gpu_program_manager.hpp"
#include "drape/overlay_handle.hpp"
#include "drape/texture_manager.hpp"

#include "geometry/screenbase.hpp"

#include "std/map.hpp"
#include "std/unique_ptr.hpp"

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

  void Build(ref_ptr<dp::GpuProgramManager> mng);
  void Render(ref_ptr<dp::GpuProgramManager> mng, ScreenBase const & screen);
  void Merge(ref_ptr<LayerRenderer> other);

  ref_ptr<dp::OverlayHandle> ProcessTapEvent(m2::PointD const & pt);

private:
  void DestroyRenderers();

  friend class LayerCacher;
  void AddShapeRenderer(Skin::ElementName name, drape_ptr<ShapeRenderer> && shape);

private:
  typedef map<Skin::ElementName, drape_ptr<ShapeRenderer> > TRenderers;
  TRenderers m_renderers;
};

struct Handlers
{
  dp::TOverlayHandler m_onCompassTapped;
  dp::TOverlayHandler m_onDownloadMapTapped;
  dp::TOverlayHandler m_onDownloadMapRoutingTapped;
  dp::TOverlayHandler m_onTryAgainTapped;

  void Reset();
};

class LayerCacher
{
public:
  LayerCacher(string const & deviceType);

  void Resize(int w, int h);
  /// @param names - can be combinations of single flags, or equal AllElements
  drape_ptr<LayerRenderer> Recache(Skin::ElementName names, ref_ptr<dp::TextureManager> textures, Handlers const & handlers);

private:
  Position GetPos(Skin::ElementName name);

private:
  unique_ptr<Skin> m_skin;
};

}

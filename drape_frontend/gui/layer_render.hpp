#pragma once

#include "skin.hpp"
#include "shape.hpp"

#include "drape/gpu_program_manager.hpp"
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
  void SetLayout(gui::TWidgetsLayoutInfo const & info);

  bool OnTouchDown(m2::RectD const & touchArea);
  void OnTouchUp(m2::RectD const & touchArea);
  void OnTouchCancel(m2::RectD const & touchArea);

private:
  void DestroyRenderers();

  friend class LayerCacher;
  void AddShapeRenderer(EWidget widget, drape_ptr<ShapeRenderer> && shape);

private:
  typedef map<EWidget, drape_ptr<ShapeRenderer> > TRenderers;
  TRenderers m_renderers;

  ref_ptr<gui::Handle> m_activeOverlay;
};

class LayerCacher
{
public:
  drape_ptr<LayerRenderer> RecacheWidgets(TWidgetsInitInfo const & initInfo,
                                          TWidgetsSizeInfo & sizeInfo,
                                          ref_ptr<dp::TextureManager> textures);
  drape_ptr<LayerRenderer> RecacheCountryStatus(ref_ptr<dp::TextureManager> texMng);

private:
  m2::PointF CacheCompass(Position const & position, ref_ptr<LayerRenderer> renderer, ref_ptr<dp::TextureManager> textures);
  m2::PointF CacheRuler(Position const & position, ref_ptr<LayerRenderer> renderer, ref_ptr<dp::TextureManager> textures);
  m2::PointF CacheCopyright(Position const & position, ref_ptr<LayerRenderer> renderer, ref_ptr<dp::TextureManager> textures);
  m2::PointF CacheScaleLabel(Position const & position, ref_ptr<LayerRenderer> renderer, ref_ptr<dp::TextureManager> textures);
};

}

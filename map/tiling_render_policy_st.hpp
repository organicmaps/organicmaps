#pragma once

#include "render_policy.hpp"
#include "tile_renderer.hpp"
#include "coverage_generator.hpp"
#include "tiler.hpp"
#include "screen_coverage.hpp"
#include "render_policy.hpp"

#include "../yg/info_layer.hpp"
#include "../std/scoped_ptr.hpp"

namespace yg
{
  namespace gl
  {
    class Screen;
    class RenderContext;
  }
  class ResourceManager;
}

class WindowHandle;

class TilingRenderPolicyST : public RenderPolicy
{
private:

  scoped_ptr<TileRenderer> m_tileRenderer;

  scoped_ptr<CoverageGenerator> m_coverageGenerator;

  ScreenBase m_currentScreen;

  bool m_isScaling;
  int m_maxTilesCount;

  /// --- COPY/PASTE HERE ---

  ThreadedList<yg::gl::Renderer::Packet> m_glQueue;
  list<yg::gl::Renderer::Packet> m_frameGLQueue;

  threads::Condition m_glCondition;

  shared_ptr<yg::gl::Screen::BaseState> m_curState;

  shared_ptr<yg::gl::Renderer::BaseState> m_state;

  void ProcessRenderQueue(list<yg::gl::Renderer::Packet> & renderQueue, int maxPackets);

  bool m_IsDebugging;

  void RenderQueuedCommands(yg::gl::Screen * screen);

protected:

  TileRenderer & GetTileRenderer();

public:

  TilingRenderPolicyST(VideoTimer * videoTimer,
                       bool useDefaultFB,
                       yg::ResourceManager::Params const & rmParams,
                       shared_ptr<yg::gl::RenderContext> const & primaryRC);

  void BeginFrame(shared_ptr<PaintEvent> const & ev, ScreenBase const & s);
  void DrawFrame(shared_ptr<PaintEvent> const & ev, ScreenBase const & s);
  void EndFrame(shared_ptr<PaintEvent> const & ev, ScreenBase const & s);

  virtual void StartScale();
  virtual void StopScale();

  bool NeedRedraw() const;
  bool IsTiling() const;

  void SetRenderFn(TRenderFn renderFn);
};

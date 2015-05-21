#pragma once

#include "drawer.hpp"

#include "graphics/screen.hpp"

#include "std/unique_ptr.hpp"

class ScreenBase;

namespace drule
{
  class BaseRule;
}

namespace graphics
{
  namespace gl
  {
    class FrameBuffer;
    class BaseTexture;
  }

  class ResourceManager;
}

class GPUDrawer : public Drawer
{
  using TBase = Drawer;

public:
  struct Params : Drawer::Params
  {
    graphics::Screen::Params m_screenParams;
  };

  GPUDrawer(Params const & params);

  graphics::Screen * Screen() const;

  void BeginFrame();
  void EndFrame();
  void OnSize(int w, int h);
  graphics::GlyphCache * GetGlyphCache() override;

  static graphics::Screen * GetScreen(Drawer * drawer);

protected:
  typedef shared_ptr<graphics::gl::BaseTexture> texture_t;
  typedef shared_ptr<graphics::gl::FrameBuffer> frame_buffer_t;

  int ThreadSlot() const;

  void DrawSymbol(m2::PointD const & pt,
                  graphics::EPosition pos,
                  di::DrawRule const & rule) override;

  void DrawCircle(m2::PointD const & pt,
                  graphics::EPosition pos,
                  di::DrawRule const & rule) override;

  void DrawCircledSymbol(m2::PointD const & pt,
                         graphics::EPosition pos,
                         di::DrawRule const & symbolRule,
                         di::DrawRule const & circleRule) override;

  void DrawPath(di::PathInfo const & path,
                di::DrawRule const * rules,
                size_t count) override;

  void DrawArea(di::AreaInfo const & area,
                di::DrawRule const & rule) override;

  void DrawText(m2::PointD const & pt,
                graphics::EPosition pos,
                di::FeatureStyler const & fs,
                di::DrawRule const & rule) override;

  void DrawPathText(di::PathInfo const & info,
                    di::FeatureStyler const & fs,
                    di::DrawRule const & rule) override;

  void DrawPathNumber(di::PathInfo const & path,
                      di::FeatureStyler const & fs) override;

private:
  static void ClearResourceCache(size_t threadSlot, uint8_t pipelineID);

  unique_ptr<graphics::Screen> const m_pScreen;
};

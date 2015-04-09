#pragma once

#include "map/feature_info.hpp"

#include "graphics/color.hpp"
#include "graphics/screen.hpp"

#include "std/list.hpp"
#include "std/string.hpp"
#include "std/shared_ptr.hpp"
#include "std/map.hpp"

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


class Drawer
{
  double m_visualScale;
  int m_level;

  unique_ptr<graphics::Screen> const m_pScreen;

  static void ClearResourceCache(size_t threadSlot, uint8_t pipelineID);

protected:

  void drawSymbol(m2::PointD const & pt,
                  graphics::EPosition pos,
                  di::DrawRule const & rule,
                  FeatureID const & id);

  void drawCircle(m2::PointD const & pt,
                  graphics::EPosition pos,
                  di::DrawRule const & rule,
                  FeatureID const & id);

  void drawCircledSymbol(m2::PointD const & pt,
                         graphics::EPosition pos,
                         di::DrawRule const & symbolRule,
                         di::DrawRule const & circleRule,
                         FeatureID const & id);

  void drawPath(di::PathInfo const & path,
                di::DrawRule const * rules,
                size_t count);

  void drawArea(di::AreaInfo const & area,
                di::DrawRule const & rule);

  void drawText(m2::PointD const & pt,
                graphics::EPosition pos,
                di::FeatureStyler const & fs,
                di::DrawRule const & rule,
                FeatureID const & id);

  void drawPathText(di::PathInfo const & info,
                    di::FeatureStyler const & fs,
                    di::DrawRule const & rule);

  void drawPathNumber(di::PathInfo const & path,
                      di::FeatureStyler const & fs);

  typedef shared_ptr<graphics::gl::BaseTexture> texture_t;
  typedef shared_ptr<graphics::gl::FrameBuffer> frame_buffer_t;

public:

  struct Params : graphics::Screen::Params
  {
    double m_visualScale;
    Params();
  };

  Drawer(Params const & params = Params());

  double VisualScale() const;
  void SetScale(int level);

  int ThreadSlot() const;

  graphics::Screen * screen() const;

  void drawSymbol(m2::PointD const & pt,
                  string const & symbolName,
                  graphics::EPosition pos,
                  double depth);

  void beginFrame();
  void endFrame();

  void clear(graphics::Color const & c = graphics::Color(187, 187, 187, 255),
             bool clearRT = true,
             float depth = 1.0f,
             bool clearDepth = true);

  void onSize(int w, int h);

  void Draw(di::FeatureInfo const & fi);
};

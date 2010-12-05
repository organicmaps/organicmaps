#pragma once

#include "draw_info.hpp"

#include "../yg/color.hpp"

#include "../std/list.hpp"
#include "../std/string.hpp"
#include "../std/shared_ptr.hpp"

class ScreenBase;
namespace drule { class BaseRule; }

namespace yg
{
  namespace gl
  {
    class Screen;
    class FrameBuffer;
    class BaseTexture;
  }

  class ResourceManager;
  class Skin;
}

namespace di
{
  class DrawInfo
  {
  public:
    DrawInfo(string const & name) : m_name(name) {}

    list<di::PathInfo> m_pathes;
    list<di::AreaInfo> m_areas;
    m2::PointD m_point;

    string m_name;
  };
}

class DrawerYG
{
private:
  typedef drule::BaseRule const * rule_ptr_t;

  double m_scale;
  double m_visualScale;

  shared_ptr<yg::gl::Screen> m_pScreen;
  shared_ptr<yg::Skin> m_pSkin;

  static void ClearSkinPage(uint8_t pageID);

  uint8_t get_font_size(rule_ptr_t pRule);

protected:
  void drawSymbol(m2::PointD const & pt, rule_ptr_t pRule, int depth);
  void drawPath(vector<m2::PointD> const & pts, rule_ptr_t pRule, int depth);
  void drawArea(vector<m2::PointD> const & pts, rule_ptr_t pRule, int depth);

  void drawText(m2::PointD const & pt, string const & name, rule_ptr_t pRule, int depth);
  void drawPathText(vector<m2::PointD> const & pts, double pathLength, string const & name, rule_ptr_t pRule, int depth);

  typedef shared_ptr<yg::gl::BaseTexture> texture_t;
  typedef shared_ptr<yg::gl::FrameBuffer> frame_buffer_t;

public:
  typedef yg::gl::Screen screen_t;

  DrawerYG(shared_ptr<yg::ResourceManager> const & tm, string const & skinName, bool isAntiAliased);

  //render_target_t renderTarget() const;
  void setFrameBuffer(frame_buffer_t frameBuffer);
  //frame_buffer_t frameBuffer() const;

  void beginFrame();
  void endFrame();

  void clear(yg::Color const & c = yg::Color(192, 192, 192, 255), bool clearRT = true, float depth = 1.0f, bool clearDepth = true);

  void onSize(int w, int h);

  void drawStats(double duration, int scale, double lat, double lng);

  shared_ptr<yg::gl::Screen> screen() const;

  void SetVisualScale(double visualScale);
  void SetScale(int level);
  void Draw(di::DrawInfo const * pInfo, rule_ptr_t pRule, int depth);
};

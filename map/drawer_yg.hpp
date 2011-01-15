#pragma once

#include "draw_info.hpp"

#include "../yg/color.hpp"
#include "../yg/screen.hpp"

#include "../std/list.hpp"
#include "../std/string.hpp"
#include "../std/shared_ptr.hpp"
#include "../std/map.hpp"


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

  uint8_t get_text_font_size(rule_ptr_t pRule) const;
  uint8_t get_pathtext_font_size(rule_ptr_t pRule) const;

  typedef map<string, list<m2::RectD> > org_map_t;
  org_map_t m_pathsOrg;

protected:
  void drawSymbol(m2::PointD const & pt, rule_ptr_t pRule, int depth);
  void drawPath(vector<m2::PointD> const & pts, rule_ptr_t pRule, int depth);
  void drawArea(vector<m2::PointD> const & pts, rule_ptr_t pRule, int depth);

  void drawText(m2::PointD const & pt, string const & name, rule_ptr_t pRule, int depth);
  bool drawPathText(di::PathInfo const & info, string const & name, uint8_t fontSize, int depth);

  typedef shared_ptr<yg::gl::BaseTexture> texture_t;
  typedef shared_ptr<yg::gl::FrameBuffer> frame_buffer_t;

public:
  typedef yg::gl::Screen screen_t;

  struct Params : screen_t::Params
  {
    size_t m_dynamicPagesCount;
    size_t m_textPagesCount;
    Params();
  };

  typedef Params params_t;

  DrawerYG(string const & skinName, params_t const & params = params_t());

  void drawSymbol(m2::PointD const & pt, string const & symbolName, int depth);

  void beginFrame();
  void endFrame();

  void clear(yg::Color const & c = yg::Color(192, 192, 192, 255), bool clearRT = true, float depth = 1.0f, bool clearDepth = true);

  void onSize(int w, int h);

  shared_ptr<yg::gl::Screen> screen() const;

  void SetVisualScale(double visualScale);
  void SetScale(int level);
  void Draw(di::DrawInfo const * pInfo, rule_ptr_t pRule, int depth);
};

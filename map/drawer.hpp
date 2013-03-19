#pragma once

#include "draw_info.hpp"

#include "../graphics/color.hpp"
#include "../graphics/screen.hpp"

#include "../std/list.hpp"
#include "../std/string.hpp"
#include "../std/shared_ptr.hpp"
#include "../std/map.hpp"


class ScreenBase;
namespace drule { class BaseRule; }

namespace graphics
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
    DrawInfo(string const & name,
             string const & secondaryName,
             string const & road,
             double rank);

    list<di::PathInfo> m_pathes;
    list<di::AreaInfo> m_areas;
    m2::PointD m_point;

    string m_name;
    string m_secondaryName;
    string m_road;
    double m_rank;

    string const GetPathName() const;
  };

  struct DrawRule
  {
    typedef drule::BaseRule const * rule_ptr_t;

    rule_ptr_t m_rule;
    double m_depth;
    bool m_transparent;

    DrawRule() : m_rule(0) {}
    DrawRule(rule_ptr_t p, double d, bool tr) : m_rule(p), m_depth(d), m_transparent(tr) {}

    uint32_t GetID(size_t threadSlot) const;
    void SetID(size_t threadSlot, uint32_t id) const;
  };
}

class Drawer
{
  typedef di::DrawRule::rule_ptr_t rule_ptr_t;

  double m_visualScale;
  int m_level;

  shared_ptr<graphics::Screen> m_pScreen;
  shared_ptr<graphics::Skin> m_pSkin;

  static void ClearResourceCache(size_t threadSlot, uint8_t pipelineID);

  typedef pair<size_t, uint32_t> FeatureID;

protected:
  void drawSymbol(m2::PointD const & pt, rule_ptr_t pRule,
                  graphics::EPosition pos, double depth, FeatureID const & id);
  void drawCircle(m2::PointD const & pt, rule_ptr_t pRule,
                  graphics::EPosition pos, double depth, FeatureID const & id);
  void drawPath(di::PathInfo const & info, di::DrawRule const * rules, size_t count);
  void drawArea(vector<m2::PointD> const & pts, rule_ptr_t pRule, double depth);

  void drawText(m2::PointD const & pt, di::DrawInfo const * pInfo, rule_ptr_t pRule,
                graphics::EPosition pos, double depth, FeatureID const & id);
  bool drawPathText(di::PathInfo const & info, di::DrawInfo const * pInfo, rule_ptr_t pRule, double depth);
  void drawPathNumber(di::PathInfo const & path, di::DrawInfo const * pInfo);

  typedef shared_ptr<graphics::gl::BaseTexture> texture_t;
  typedef shared_ptr<graphics::gl::FrameBuffer> frame_buffer_t;

public:

  struct Params : graphics::Screen::Params
  {
    double m_visualScale;
    Params();
  };

  Drawer(Params const & params = Params());

  void drawSymbol(m2::PointD const & pt, string const & symbolName, graphics::EPosition pos, double depth);

  void beginFrame();
  void endFrame();

  void clear(graphics::Color const & c = graphics::Color(187, 187, 187, 255), bool clearRT = true, float depth = 1.0f, bool clearDepth = true);

  void onSize(int w, int h);

  shared_ptr<graphics::Screen> screen() const;

  double VisualScale() const;
  void SetScale(int level);

  int ThreadSlot() const;

  void Draw(di::DrawInfo const * pInfo,
            di::DrawRule const * rules, size_t count,
            FeatureID const & id);

  bool filter_text_size(rule_ptr_t pRule) const;
  uint8_t get_text_font_size(rule_ptr_t pRule) const;
};

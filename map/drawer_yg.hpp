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
    int m_depth;
    bool m_transparent;

    DrawRule() : m_rule(0) {}
    DrawRule(rule_ptr_t p, int d, bool tr) : m_rule(p), m_depth(d), m_transparent(tr) {}

    uint32_t GetID(size_t threadID) const;
    void SetID(size_t threadID, uint32_t id) const;
  };
}

class DrawerYG
{
  typedef di::DrawRule::rule_ptr_t rule_ptr_t;

  double m_visualScale;
  int m_level;
  size_t m_threadID;

  shared_ptr<yg::gl::Screen> m_pScreen;
  shared_ptr<yg::Skin> m_pSkin;

  static void ClearSkinPage(size_t threadID, uint8_t pipelineID);

  typedef pair<size_t, uint32_t> FeatureID;

protected:
  void drawSymbol(m2::PointD const & pt, rule_ptr_t pRule,
                  yg::EPosition pos, int depth, FeatureID const & id);
  void drawCircle(m2::PointD const & pt, rule_ptr_t pRule,
                  yg::EPosition pos, int depth, FeatureID const & id);
  void drawPath(di::PathInfo const & info, di::DrawRule const * rules, size_t count);
  void drawArea(vector<m2::PointD> const & pts, rule_ptr_t pRule, int depth);

  void drawText(m2::PointD const & pt, di::DrawInfo const * pInfo, rule_ptr_t pRule,
                yg::EPosition pos, int depth, FeatureID const & id);
  bool drawPathText(di::PathInfo const & info, di::DrawInfo const * pInfo, rule_ptr_t pRule, int depth);
  void drawPathNumber(di::PathInfo const & path, di::DrawInfo const * pInfo);

  typedef shared_ptr<yg::gl::BaseTexture> texture_t;
  typedef shared_ptr<yg::gl::FrameBuffer> frame_buffer_t;

public:
  struct Params : yg::gl::Screen::Params
  {
    size_t m_threadID;
    double m_visualScale;
    shared_ptr<yg::Skin> m_skin;
    Params();
  };

  DrawerYG(Params const & params = Params());

  void drawSymbol(m2::PointD const & pt, string const & symbolName, yg::EPosition pos, int depth);

  void beginFrame();
  void endFrame();

  void clear(yg::Color const & c = yg::Color(187, 187, 187, 255), bool clearRT = true, float depth = 1.0f, bool clearDepth = true);

  void onSize(int w, int h);

  shared_ptr<yg::gl::Screen> screen() const;

  double VisualScale() const;
  void SetScale(int level);

  void Draw(di::DrawInfo const * pInfo,
            di::DrawRule const * rules, size_t count,
            FeatureID const & id);

  bool filter_text_size(rule_ptr_t pRule) const;
  uint8_t get_text_font_size(rule_ptr_t pRule) const;
};

#pragma once

#include "graphics/defines.hpp"
#include "graphics/font_desc.hpp"

#include "indexer/feature_decl.hpp"

#include "geometry/point2d.hpp"

#include "std/function.hpp"

namespace di
{
  struct DrawRule;
  class PathInfo;
  class AreaInfo;
  struct FeatureStyler;
  struct FeatureInfo;
}

namespace graphics { class GlyphCache; }

class Drawer
{
  double m_visualScale;
  int m_level;
  FeatureID m_currentFeatureID;

protected:

  virtual void DrawSymbol(m2::PointD const & pt,
                          graphics::EPosition pos,
                          di::DrawRule const & rule) = 0;

  virtual void DrawCircle(m2::PointD const & pt,
                          graphics::EPosition pos,
                          di::DrawRule const & rule) = 0;

  virtual void DrawCircledSymbol(m2::PointD const & pt,
                                 graphics::EPosition pos,
                                 di::DrawRule const & symbolRule,
                                 di::DrawRule const & circleRule) = 0;

  virtual void DrawPath(di::PathInfo const & path,
                        di::DrawRule const * rules,
                        size_t count) = 0;

  virtual void DrawArea(di::AreaInfo const & area,
                        di::DrawRule const & rule) = 0;

  virtual void DrawText(m2::PointD const & pt,
                        graphics::EPosition pos,
                        di::FeatureStyler const & fs,
                        di::DrawRule const & rule) = 0;

  virtual void DrawPathText(di::PathInfo const & info,
                            di::FeatureStyler const & fs,
                            di::DrawRule const & rule) = 0;

  virtual void DrawPathNumber(di::PathInfo const & path,
                              di::FeatureStyler const & fs) = 0;

  void DrawFeatureStart(FeatureID const & id);
  void DrawFeatureEnd(FeatureID const & id);
  FeatureID const & GetCurrentFeatureID() const;

  using TRoadNumberCallbackFn = function<void (m2::PointD const & pt, graphics::FontDesc const & font, string const & text)>;
  void GenerateRoadNumbers(di::PathInfo const & path, di::FeatureStyler const & fs, TRoadNumberCallbackFn const & fn);

public:

  struct Params
  {
    double m_visualScale;
    Params();
  };

  Drawer(Params const & params = Params());
  virtual ~Drawer() {}

  double VisualScale() const;
  void SetScale(int level);

  virtual graphics::GlyphCache * GetGlyphCache() = 0;

  void Draw(di::FeatureInfo const & fi);
};

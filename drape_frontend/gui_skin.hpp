#pragma once

#include "../coding/reader.hpp"

#include "../drape/drape_global.hpp"
#include "../geometry/point2d.hpp"

#include "../std/map.hpp"

namespace df
{

struct GuiPosition
{
  GuiPosition(m2::PointF const & pt, dp::Anchor anchor)
    : m_pixelPivot(pt)
    , m_anchor(anchor) {}

  m2::PointF const m_pixelPivot;
  dp::Anchor const m_anchor;
};

class PositionResolver
{
public:
  PositionResolver() = default;

  GuiPosition Resolve(int w, int h) const;
  void SetAnchorVertical(dp::Anchor anchor);
  void SetAnchorHorizontal(dp::Anchor anchor);
  void SetRelativeVertical(dp::Anchor anchor);
  void SetRelativeHorizontal(dp::Anchor anchor);
  void SetOffsetX(float x);
  void SetOffsetY(float y);

private:
  dp::Anchor m_elementAnchor = dp::Center;
  dp::Anchor m_resolveAnchor = dp::Center;
  m2::PointF m_offset = m2::PointF::Zero();
};

class GuiSkin
{
public:
  enum class GuiElement
  {
    CountryStatus,
    Ruler,
    Compass,
    Copyright
  };

  GuiSkin(ReaderPtr<Reader> const & reader);

  GuiPosition ResolvePosition(GuiElement name);
  void Resize(int w, int h);

private:
  /// TResolversPair.first - Portrait (when weight < height)
  /// TResolversPair.second - Landscape (when weight >= height)
  typedef pair<PositionResolver, PositionResolver> TResolversPair;
  map<GuiElement, TResolversPair> m_resolvers;

  int m_displayWidth;
  int m_displayHeight;
};

ReaderPtr<Reader> ResolveGuiSkinFile(string const & deviceType);

}

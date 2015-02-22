#pragma once

#include "../drape/drape_global.hpp"
#include "../geometry/point2d.hpp"
#include "../coding/reader.hpp"

#include "../std/map.hpp"

namespace gui
{

struct Position
{
  Position() : m_pixelPivot(m2::PointF::Zero()), m_anchor(dp::Center) {}
  Position(m2::PointF const & pt, dp::Anchor anchor)
    : m_pixelPivot(pt)
    , m_anchor(anchor) {}

  m2::PointF m_pixelPivot;
  dp::Anchor m_anchor;
};

class PositionResolver
{
public:
  Position Resolve(int w, int h, double vs) const;
  void AddAnchor(dp::Anchor anchor);
  void AddRelative(dp::Anchor anchor);
  void SetOffsetX(float x);
  void SetOffsetY(float y);

private:
  dp::Anchor m_elementAnchor = dp::Center;
  dp::Anchor m_resolveAnchor = dp::Center;
  m2::PointF m_offset = m2::PointF::Zero();
};

class Skin
{
public:
  enum class ElementName
  {
    CountryStatus,
    Ruler,
    Compass,
    Copyright
  };

  explicit Skin(ReaderPtr<Reader> const & reader, double visualScale);

  Position ResolvePosition(ElementName name);
  void Resize(int w, int h);
  int GetWidth() const { return m_displayWidth; }
  int GetHeight() const { return m_displayHeight; }

private:
  /// TResolversPair.first - Portrait (when weight < height)
  /// TResolversPair.second - Landscape (when weight >= height)
  typedef pair<PositionResolver, PositionResolver> TResolversPair;
  map<ElementName, TResolversPair> m_resolvers;

  int m_displayWidth;
  int m_displayHeight;
  double m_vs;
};

ReaderPtr<Reader> ResolveGuiSkinFile(string const & deviceType);

}

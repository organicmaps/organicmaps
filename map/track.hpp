#pragma once

#include "../geometry/polyline2d.hpp"
#include "../graphics/color.hpp"
#include "events.hpp"

class Track
{
public:
  typedef m2::Polyline2d<double> PolylineD;

  Track()
    : m_isVisible(true), m_name("unnamed_track"), m_width(10),
      m_color(graphics::Color::fromARGB(0xFFFF0000)) {}

  explicit Track(PolylineD & polyline)
    : m_isVisible(true), m_name("unnamed_track"), m_width(10),
      m_color(graphics::Color::fromARGB(0xFFFF0000)),
      m_polyline(polyline) {}

  void Draw(shared_ptr<PaintEvent> const & e);

  size_t Size();
  m2::RectD GetLimitRect();

  bool IsVisible() { return m_isVisible; }
  void SetVisible(bool visible) { m_isVisible = visible; }

  size_t GetWidth() { return m_width; }
  void SetWidth(size_t width) { m_width = width; }

  graphics::Color GetColor() { return m_color; }
  void SetColor(graphics::Color color) { m_color = color; }

private:
  bool m_isVisible;
  string m_name;
  size_t m_width;
  graphics::Color m_color;

  PolylineD m_polyline;
};

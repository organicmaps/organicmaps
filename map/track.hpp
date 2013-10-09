#pragma once

#include "../geometry/polyline2d.hpp"
#include "../geometry/screenbase.hpp"

#include "../graphics/color.hpp"

#include "../std/shared_ptr.hpp"
#include "../std/noncopyable.hpp"


class Navigator;
class PaintEvent;

namespace graphics
{
  class Screen;
  class DisplayList;
  class PaintEvent;
}


class Track : private noncopyable
{
public:
  ~Track();

  typedef m2::PolylineD PolylineD;

  Track()
    : m_isVisible(true), m_name("unnamed_track"), m_width(5),
      m_color(graphics::Color::fromARGB(0xFFFF0000)),
      m_dList(0)
  {}

  explicit Track(PolylineD const & polyline)
    : m_isVisible(true), m_name("unnamed_track"), m_width(5),
      m_color(graphics::Color::fromARGB(0xFFFF0000)),
      m_polyline(polyline),
      m_dList(0)
  {}

  /// @note Move semantics is used here.
  Track * CreatePersistent();

  void Draw(shared_ptr<PaintEvent> const & e);
  void UpdateDisplayList(Navigator & navigator, graphics::Screen * dListScreen);
  void DeleteDisplayList();

  size_t Size() const;
  m2::RectD GetLimitRect() const;

  /// @name Simple Getters-Setter
  //@{
  bool IsVisible() const        { return m_isVisible; }
  void SetVisible(bool visible) { m_isVisible = visible; }

  size_t GetWidth() const       { return m_width; }
  void   SetWidth(size_t width) { m_width = width; }

  graphics::Color GetColor() const                { return m_color; }
  void            SetColor(graphics::Color color) { m_color = color; }

  string GetName() const              { return m_name; }
  void   SetName(string const & name) { m_name = name; }

  double GetLength() const { return m_polyline.GetLength(); }
  PolylineD const & GetPolyline() const { return m_polyline; }

  double GetShortestSquareDistance(m2::PointD const & point) const;
  //@}

  void Swap(Track & rhs);

private:
  bool m_isVisible;
  string m_name;
  size_t m_width;
  graphics::Color m_color;

  PolylineD m_polyline;

  ScreenBase m_screenBase;
  graphics::DisplayList * m_dList;

  bool IsViewportChanged(ScreenBase const & sb) const;
  inline bool HasDisplayList() const { return m_dList != 0; }
};

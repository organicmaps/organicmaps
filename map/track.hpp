#pragma once

#include "../geometry/polyline2d.hpp"
#include "../geometry/screenbase.hpp"

#include "../graphics/color.hpp"

#include "../std/noncopyable.hpp"


class Navigator;
namespace graphics
{
  class Screen;
  class DisplayList;
}


class Track : private noncopyable
{
  typedef math::Matrix<double, 3, 3> MatrixT;

public:
  ~Track();

  typedef m2::PolylineD PolylineD;

  Track()
    : m_isVisible(true), m_width(5),
      m_color(graphics::Color::fromARGB(0xFFFF0000)),
      m_dList(0)
  {}

  explicit Track(PolylineD const & polyline)
    : m_isVisible(true), m_width(5),
      m_color(graphics::Color::fromARGB(0xFFFF0000)),
      m_polyline(polyline),
      m_dList(0)
  {
    ASSERT_GREATER(polyline.GetSize(), 1, ());

    m_rect = m_polyline.GetLimitRect();
  }

  /// @note Move semantics is used here.
  Track * CreatePersistent();

  void Draw(graphics::Screen * pScreen, MatrixT const & matrix) const;
  void CreateDisplayList(graphics::Screen * dlScreen, MatrixT const & matrix) const;
  void DeleteDisplayList() const;
  bool HasDisplayList() const { return m_dList != 0; }

  /// @name Simple Getters-Setter
  //@{
  bool IsVisible() const        { return m_isVisible; }
  void SetVisible(bool visible) { m_isVisible = visible; }

  size_t GetWidth() const { return m_width; }
  void SetWidth(size_t width) { m_width = width; }

  graphics::Color GetColor() const { return m_color; }
  void SetColor(graphics::Color color) { m_color = color; }

  string const & GetName() const { return m_name; }
  void SetName(string const & name) { m_name = name; }

  PolylineD const & GetPolyline() const { return m_polyline; }
  m2::RectD const & GetLimitRect() const { return m_rect; }
  //@}

  double GetLengthMeters() const;
  double GetShortestSquareDistance(m2::PointD const & point) const;

  void Swap(Track & rhs);

private:
  bool m_isVisible;
  string m_name;
  size_t m_width;
  graphics::Color m_color;

  PolylineD m_polyline;
  m2::RectD m_rect;

  mutable graphics::DisplayList * m_dList;
};

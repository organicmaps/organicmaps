#pragma once

#include "../geometry/polyline2d.hpp"
#include "../geometry/screenbase.hpp"

#include "../graphics/color.hpp"
#include "../graphics/defines.hpp"

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
  Track() {}
  ~Track();

  typedef m2::PolylineD PolylineD;

  explicit Track(PolylineD const & polyline)
    : m_isVisible(true),
      m_polyline(polyline)
  {
    ASSERT_GREATER(polyline.GetSize(), 1, ());

    m_rect = m_polyline.GetLimitRect();
  }

  /// @note Move semantics is used here.
  Track * CreatePersistent();
  float GetMainWidth() const;
  graphics::Color const & GetMainColor() const;

  void Draw(graphics::Screen * pScreen, MatrixT const & matrix) const;
  void CreateDisplayList(graphics::Screen * dlScreen, MatrixT const & matrix) const;
  void DeleteDisplayList() const;
  bool HasDisplayList() const { return m_dList != nullptr; }

  /// @name Simple Getters-Setter
  //@{
  bool IsVisible() const        { return m_isVisible; }
  void SetVisible(bool visible) { m_isVisible = visible; }

  struct TrackOutline
  {
    float m_lineWidth;
    graphics::Color m_color;
  };

  void AddOutline(TrackOutline const * outline, size_t arraySize);

  string const & GetName() const { return m_name; }
  void SetName(string const & name) { m_name = name; }

  PolylineD const & GetPolyline() const { return m_polyline; }
  m2::RectD const & GetLimitRect() const { return m_rect; }
  //@}

  void AddClosingSymbol(bool isBeginSymbol, string const & symbolName,
                        graphics::EPosition pos, double depth);

  double GetLengthMeters() const;

  void Swap(Track & rhs);

private:
  bool m_isVisible = false;
  string m_name;

  vector<TrackOutline> m_outlines;

  struct ClosingSymbol
  {
    ClosingSymbol(string const & iconName, graphics::EPosition pos, double depth)
      : m_iconName(iconName), m_position(pos), m_depth(depth) {}
    string m_iconName;
    graphics::EPosition m_position;
    double m_depth;
  };

  vector<ClosingSymbol> m_beginSymbols;
  vector<ClosingSymbol> m_endSymbols;

  PolylineD m_polyline;
  m2::RectD m_rect;

  mutable graphics::DisplayList * m_dList = nullptr;
};

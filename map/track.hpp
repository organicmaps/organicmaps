#pragma once

#include "drape/drape_global.hpp"
#include "drape/color.hpp"

#include "geometry/polyline2d.hpp"
#include "geometry/screenbase.hpp"

#include "std/noncopyable.hpp"

#include "base/buffer_vector.hpp"


class Navigator;
namespace location
{
  class RouteMatchingInfo;
}

template <class T> class DoLeftProduct
{
  T const & m_t;
public:
  DoLeftProduct(T const & t) : m_t(t) {}
  template <class X> X operator() (X const & x) const { return x * m_t; }
};

typedef math::Matrix<double, 3, 3> MatrixT;
typedef buffer_vector<m2::PointD, 32> PointContainerT;

class Track : private noncopyable
{
public:
  typedef m2::PolylineD PolylineD;

  Track() {}
  virtual ~Track();

  explicit Track(PolylineD const & polyline)
    : m_polyline(polyline)
  {
    ASSERT_GREATER(polyline.GetSize(), 1, ());

    m_rect = m_polyline.GetLimitRect();
  }

  /// @note Move semantics is used here.
  virtual Track * CreatePersistent();
  float GetMainWidth() const;
  dp::Color const & GetMainColor() const;


  /// @TODO UVR
  //virtual void Draw(graphics::Screen * pScreen, MatrixT const & matrix) const;
  //virtual void CreateDisplayList(graphics::Screen * dlScreen, MatrixT const & matrix, bool isScaleChanged,
                         int, double, location::RouteMatchingInfo const &) const;
  //virtual void CleanUp() const;
  //virtual bool HasDisplayLists() const;

  /// @name Simple Getters-Setter
  //@{

  struct TrackOutline
  {
    float m_lineWidth;
    dp::Color m_color;
  };

  void AddOutline(TrackOutline const * outline, size_t arraySize);

  string const & GetName() const { return m_name; }
  void SetName(string const & name) { m_name = name; }

  PolylineD const & GetPolyline() const { return m_polyline; }
  m2::RectD const & GetLimitRect() const { return m_rect; }
  //@}
  double GetLengthMeters() const;

protected:
  graphics::DisplayList * GetDisplayList() const { return m_dList; }
  void SetDisplayList(graphics::DisplayList * dl) const { m_dList = dl; }
  void CreateDisplayListPolyline(graphics::Screen * dlScreen, PointContainerT const & pts2) const;
  void Swap(Track & rhs);
  void DeleteDisplayList() const;

private:
  string m_name;

  vector<TrackOutline> m_outlines;
  PolylineD m_polyline;
  m2::RectD m_rect;

  ///@TODO UVR
  //mutable graphics::DisplayList * m_dList = nullptr;
};

void TransformPolyline(Track::PolylineD const & polyline, MatrixT const & matrix, PointContainerT & pts);
void TransformAndSymplifyPolyline(Track::PolylineD const & polyline, MatrixT const & matrix,
                                  double width, PointContainerT & pts);

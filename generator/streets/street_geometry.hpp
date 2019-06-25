#pragma once

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"

#include "base/geo_object_id.hpp"

#include <list>
#include <memory>
#include <utility>
#include <vector>

#include <boost/optional.hpp>

namespace generator
{
namespace streets
{
// Pin is central position of street on the map.
struct Pin
{
  m2::PointD m_position;
  base::GeoObjectId m_osmId;
};

class HighwayGeometry;
class BindingsGeometry;

class StreetGeometry
{
public:
  Pin GetOrChoosePin() const;
  // Bbox may be the minimum bounding box with feature type margin.
  m2::RectD GetBbox() const;

  void SetPin(Pin && pin);
  void AddHighwayLine(base::GeoObjectId const & osmId, std::vector<m2::PointD> const & line);
  void AddHighwayArea(base::GeoObjectId const osmId, std::vector<m2::PointD> const & border);
  // Bindings are buildings or POIs on this street (that is the objects that have this street
  // in their address).
  void AddBinding(base::GeoObjectId const & osmId, m2::PointD const & point);

private:
  boost::optional<Pin> m_pin;
  std::unique_ptr<HighwayGeometry> m_highwayGeometry;
  std::unique_ptr<BindingsGeometry> m_bindingsGeometry;
};

class HighwayGeometry
{
public:
  Pin ChoosePin() const;
  m2::RectD const & GetBbox() const;

  void AddLine(base::GeoObjectId const & osmId, std::vector<m2::PointD> const & line);
  void AddArea(base::GeoObjectId const & osmId, std::vector<m2::PointD> const & border);

private:
  struct LineSegment
  {
    LineSegment(base::GeoObjectId const & osmId, std::vector<m2::PointD> const & points);

    double CalculateLength() const noexcept;

    base::GeoObjectId m_osmId;
    std::vector<m2::PointD> m_points;
  };

  struct Line
  {
    static constexpr double kCoordEqualityEps = 1e-5;

    // Try to append |line| to front or to back of |this| line.
    bool Concatenate(Line && other);
    void Reverse();
    // The function does not check addition of line segment to ring line.
    // Line configuration depends on addition sequences.
    // |segment| will not modify if the function returns false.
    bool Add(LineSegment && segment);
    double CalculateLength() const noexcept;

    std::list<LineSegment> m_segments;
  };

  struct MultiLine
  {
    void Add(LineSegment && lineSegment);
    // This function tries to concatenate whole |line| to another line in this multiline.
    bool Recombine(Line && line);

    std::list<Line> m_lines;
  };

  struct AreaPart
  {
    AreaPart(base::GeoObjectId const & osmId, std::vector<m2::PointD> const & polygon);

    base::GeoObjectId m_osmId;
    m2::PointD m_center;
    double m_area;
  };

  Pin ChooseMultilinePin() const;
  Pin ChooseLinePin(Line const & line, double disposeDistance) const;
  Pin ChooseAreaPin() const;
  void ExtendLimitRect(std::vector<m2::PointD> const & points);

  MultiLine m_multiLine; // TODO: cyclic graph
  std::vector<AreaPart> m_areaParts;
  m2::RectD m_limitRect;
};

class BindingsGeometry
{
public:
  Pin GetCentralBinding() const;
  m2::RectD const & GetBbox() const;

  void Add(base::GeoObjectId const & osmId, m2::PointD const & point);

private:
  void ExtendLimitRect(m2::PointD const & point);

  boost::optional<Pin> m_centralBinding;
  m2::RectD m_limitRect;
};
}  // namespace streets
}  // namespace generator

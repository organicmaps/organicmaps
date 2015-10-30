#pragma once

#include "geometry/latlon.hpp"

#include "std/string.hpp"
#include "std/utility.hpp"
#include "std/vector.hpp"

namespace routing
{
enum class PointType : char
{
  EDoubleCross = '1',
  EUnconnectedExit = '2'
};

class WarningsWriter
{
  vector<pair<ms::LatLon, PointType>> m_points;
  string const m_fileName;

public:
  WarningsWriter(string const & fileName) : m_fileName(fileName) {}
  void AddPoint(ms::LatLon const & point, PointType type);
  void Write();
};
}  // namespace routing

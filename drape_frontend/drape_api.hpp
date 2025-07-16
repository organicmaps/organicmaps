#pragma once

#include "drape_frontend/drape_engine_safe_ptr.hpp"

#include "drape/color.hpp"
#include "drape/pointers.hpp"

#include "geometry/point2d.hpp"

#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace df
{
struct DrapeApiLineData
{
  DrapeApiLineData() = default;

  DrapeApiLineData(std::vector<m2::PointD> const & points, dp::Color const & color) : m_points(points), m_color(color)
  {}

  DrapeApiLineData & ShowPoints(bool markPoints)
  {
    m_showPoints = true;
    m_markPoints = markPoints;
    return *this;
  }

  DrapeApiLineData & Width(float width)
  {
    m_width = width;
    return *this;
  }

  DrapeApiLineData & ShowId()
  {
    m_showId = true;
    return *this;
  }

  std::vector<m2::PointD> m_points;
  float m_width = 1.0f;
  dp::Color m_color;

  bool m_showPoints = false;
  bool m_markPoints = false;
  bool m_showId = false;
};

class DrapeApi
{
public:
  using TLines = std::unordered_map<std::string, DrapeApiLineData>;

  DrapeApi() = default;

  void SetDrapeEngine(ref_ptr<DrapeEngine> engine);

  void AddLine(std::string const & id, DrapeApiLineData const & data);
  void RemoveLine(std::string const & id);
  void Clear();
  void Invalidate();

private:
  DrapeEngineSafePtr m_engine;
  TLines m_lines;
};
}  // namespace df

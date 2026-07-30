#include "indexer/terrain/terrain_utils.hpp"

#include "indexer/classificator.hpp"
#include "indexer/drawing_rules.hpp"

namespace terrain
{
namespace
{
struct ClassDef
{
  char const * m_subType;
  int32_t m_metricRung;
  int32_t m_imperialRung;
};

// The isoline style classes, coarse to fine, with the altitude rungs in both units.
// The metric rungs match the class names; the imperial ladder picks the USGS-style
// round feet values closest to the metric ones in the real size, nested the same way
// (each rung divides the coarser one), so the altitude -> class resolution by
// divisibility works identically in both units.
std::array<ClassDef, 5> constexpr kClassDefs = {{
    {"step_1000", 1000, 2000},
    {"step_500", 500, 1000},
    {"step_100", 100, 500},
    {"step_50", 50, 100},
    {"step_10", 10, 20},
}};
}  // namespace

IsolinesStyle::IsolinesStyle(int zoom, measurement_utils::Units units)
{
  auto const & cl = classif();
  auto const resolve = [&](char const * subType, ClassStyle & c)
  {
    uint32_t const type = cl.GetTypeByPath({"isoline", subType});
    drule::KeysT keys;
    cl.GetObject(type)->GetSuitable(zoom, feature::GeomType::Line, keys);
    for (auto const & key : keys)
    {
      if (key.m_type == drule::line)
      {
        if (c.m_line == nullptr)
          c.m_line = drule::GetCurrentRules().Find(key)->GetLine();
      }
      else if (key.m_type == drule::pathtext && c.m_text == nullptr)
      {
        auto const * rule = drule::GetCurrentRules().Find(key)->GetPathtext();
        if (rule != nullptr && rule->primary)
          c.m_text = rule;
      }
    }
  };

  bool const imperial = units == measurement_utils::Units::Imperial;
  for (size_t i = 0; i < kClassDefs.size(); ++i)
  {
    auto & c = m_classes[i];
    c.m_rung = imperial ? kClassDefs[i].m_imperialRung : kClassDefs[i].m_metricRung;
    resolve(kClassDefs[i].m_subType, c);
    // Iterated coarse to fine: the finest visible class sets the trace step.
    if (c.m_line != nullptr)
      m_step = c.m_rung;
  }
  resolve("zero", m_zero);
}

IsolinesStyle::ClassStyle const & IsolinesStyle::GetClass(int32_t altitude) const
{
  if (altitude == 0)
    return m_zero;
  for (auto const & c : m_classes)
    if (altitude % c.m_rung == 0)
      return c;

  // Not on the ladder (an altitude of a finer trace than this style draws).
  ASSERT(false, (altitude, m_step));
  static ClassStyle const kNone;
  return kNone;
}
}  // namespace terrain

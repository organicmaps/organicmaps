#include "tiger_parser.hpp"

#include "geometry/distance_on_sphere.hpp"

#include <algorithm>

namespace tiger
{
using namespace strings;

void ParseGeometry(std::string_view s, std::vector<ms::LatLon> & geom)
{
  std::string_view constexpr prefix = "LINESTRING(";
  if (!s.ends_with(')') || !s.starts_with(prefix))
    return;

  s.remove_prefix(prefix.size());
  s.remove_suffix(1);

  ms::LatLon last;
  auto const skipPoint = [&last, &geom]()
  {
    // Check simple distance (meters) threshold.
    return !geom.empty() && ms::DistanceOnEarth(geom.back(), last) < 10.0;
  };

  Tokenize(s, ",", [&](std::string_view s)
  {
    auto i = s.find(' ');
    CHECK(i != std::string_view::npos, (s));

    CHECK(to_double(s.substr(0, i), last.m_lon), (s));
    CHECK(to_double(s.substr(i + 1), last.m_lat), (s));

    if (!skipPoint())
      geom.push_back(last);
  });

  if (skipPoint())
  {
    auto & back = geom.back();
    if (geom.size() == 1)
    {
      // Set one middle point address.
      back = ms::LatLon{(back.m_lat + last.m_lat) / 2.0, (back.m_lon + last.m_lon) / 2.0};
    }
    else
    {
      // Replace last point.
      back = last;
    }
  }
}

feature::InterpolType ParseInterpolation(std::string_view s)
{
  if (s == "all")
    return feature::InterpolType::Any;
  if (s == "odd")
    return feature::InterpolType::Odd;
  if (s == "even")
    return feature::InterpolType::Even;
  return feature::InterpolType::None;
}

bool ParseLine(std::string_view line, AddressEntry & e)
{
  // from;to;interpolation;street;city;state;postcode;geometry

  // Basic check.
  if (std::count(line.begin(), line.end(), ';') != 7)
    return false;

  TokenizeIterator<SimpleDelimiter, std::string_view::const_iterator, true /* KeepEmptyTokens */> it(line.begin(),
                                                                                                     line.end(), ";");

  e.m_from = *it;
  ++it;
  e.m_to = *it;
  ++it;
  e.m_interpol = ParseInterpolation(*it);
  ++it;
  e.m_street = *it;
  ++it;
  ++it;
  ++it;
  e.m_postcode = *it;
  ++it;
  ParseGeometry(*it, e.m_geom);

  if (e.m_interpol == feature::InterpolType::None || e.m_geom.empty())
    return false;

  // Check and order house numbers.
  auto const range = e.GetHNRange();
  if (range == AddressEntry::kInvalidRange)
    return false;

  if (range.second < range.first)
  {
    std::swap(e.m_from, e.m_to);
    std::reverse(e.m_geom.begin(), e.m_geom.end());
  }

  return true;
}

}  // namespace tiger

namespace feature
{
std::string DebugPrint(InterpolType type)
{
  switch (type)
  {
  case InterpolType::None: return "Interpol::None";
  case InterpolType::Any: return "Interpol::Any";
  case InterpolType::Odd: return "Interpol::Odd";
  case InterpolType::Even: return "Interpol::Even";
  }
  UNREACHABLE();
}
}  // namespace feature

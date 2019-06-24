#include "to_string_policy.hpp"

#include "base/assert.hpp"
#include "geometry/mercator.hpp"

#include <cstdint>
#include <memory>

#include <boost/optional.hpp>

#include "3party/jansson/myjansson.hpp"

namespace generator
{
JsonPolicy & JsonPolicy::SetRealPrecision(int32_t precision)
{
  CHECK_LESS(precision, 32, ());
  m_precision = precision;
  return *this;
}

std::string JsonPolicy::ToString(regions::NodePath const & path) const
{
  auto const & main = path.back()->GetData();
  auto geometry = base::NewJSONObject();
  ToJSONObject(*geometry, "type", "Point");
  auto coordinates = base::NewJSONArray();
  auto const tmpCenter = main.GetCenter();
  auto const center = MercatorBounds::ToLatLon({tmpCenter.get<0>(), tmpCenter.get<1>()});
  ToJSONArray(*coordinates, center.m_lon);
  ToJSONArray(*coordinates, center.m_lat);
  ToJSONObject(*geometry, "coordinates", coordinates);

  auto localeEn = base::NewJSONObject();
  auto address = base::NewJSONObject();
  boost::optional<int64_t> pid;
  for (auto const & p : path)
  {
    auto const & region = p->GetData();
    CHECK(region.GetLevel() != regions::PlaceLevel::Unknown, ());
    auto const label = GetLabel(region.GetLevel());
    CHECK(label, ());
    ToJSONObject(*address, label, region.GetName());
    if (m_extendedOutput)
    {
      ToJSONObject(*address, std::string{label} + "_i", DebugPrint(region.GetId()));
      ToJSONObject(*address, std::string{label} + "_a", region.GetArea());
      ToJSONObject(*address, std::string{label} + "_r", region.GetRank());
    }

    ToJSONObject(*localeEn, label, region.GetEnglishOrTransliteratedName());
    if (!pid && region.GetId() != main.GetId())
      pid = static_cast<int64_t>(region.GetId().GetEncodedId());
  }

  auto locales = base::NewJSONObject();
  ToJSONObject(*locales, "en", localeEn);

  auto properties = base::NewJSONObject();
  ToJSONObject(*properties, "name", main.GetName());
  ToJSONObject(*properties, "rank", main.GetRank());
  ToJSONObject(*properties, "address", address);
  ToJSONObject(*properties, "locales", locales);
  if (pid)
    ToJSONObject(*properties, "pid", *pid);
  else
    ToJSONObject(*properties, "pid", base::NewJSONNull());

  auto const & country = path.front()->GetData();
  if (auto && isoCode = country.GetIsoCode())
    ToJSONObject(*properties, "code", *isoCode);

  auto feature = base::NewJSONObject();
  ToJSONObject(*feature, "type", "Feature");
  ToJSONObject(*feature, "geometry", geometry);
  ToJSONObject(*feature, "properties", properties);

  uint32_t jsonFlags = JSON_COMPACT;
  if (m_precision >= 0)
    jsonFlags |= JSON_REAL_PRECISION(m_precision);

  return base::DumpToString(feature, jsonFlags);
}
}  // namespace generator

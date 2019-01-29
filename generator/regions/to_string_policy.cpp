#include "generator/regions/to_string_policy.hpp"

#include "geometry/mercator.hpp"

#include <cstdint>
#include <memory>

#include <boost/optional.hpp>
#include <boost/range/adaptor/reversed.hpp>

#include "3party/jansson/myjansson.hpp"

namespace generator
{
namespace regions
{
std::string JsonPolicy::ToString(Node::PtrList const & nodePtrList) const
{
  auto const & main = nodePtrList.front()->GetData();
  auto const & country = nodePtrList.back()->GetData();

  auto geometry = base::NewJSONObject();
  ToJSONObject(*geometry, "type", "Point");
  auto coordinates = base::NewJSONArray();
  auto const tmpCenter = main.GetCenter();
  auto const center = MercatorBounds::ToLatLon({tmpCenter.get<0>(), tmpCenter.get<1>()});
  ToJSONArray(*coordinates, center.lat);
  ToJSONArray(*coordinates, center.lon);
  ToJSONObject(*geometry, "coordinates", coordinates);

  auto localeEn = base::NewJSONObject();
  auto address = base::NewJSONObject();
  auto const mainLabel = main.GetLabel();
  boost::optional<int64_t> pid;
  for (auto const & p : boost::adaptors::reverse(nodePtrList))
  {

    auto const & region = p->GetData();
    auto const label = region.GetLabel();
    ToJSONObject(*address, label, region.GetName());
    if (m_extendedOutput)
    {
      ToJSONObject(*address, label + "_i", DebugPrint(region.GetId()));
      ToJSONObject(*address, label + "_a", region.GetArea());
      ToJSONObject(*address, label + "_r", region.GetRank());
    }

    ToJSONObject(*localeEn, label, region.GetEnglishOrTransliteratedName());
    if (label != mainLabel)
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

  if (country.HasIsoCode())
    ToJSONObject(*properties, "code", country.GetIsoCode());

  auto feature = base::NewJSONObject();
  ToJSONObject(*feature, "type", "Feature");
  ToJSONObject(*feature, "geometry", geometry);
  ToJSONObject(*feature, "properties", properties);

  auto const cstr = json_dumps(feature.get(), JSON_COMPACT);
  std::unique_ptr<char, JSONFreeDeleter> buffer(cstr);
  return buffer.get();
}
}  // namespace regions
}  // namespace generator

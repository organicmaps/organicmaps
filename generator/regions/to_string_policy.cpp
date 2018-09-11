#include "generator/regions/to_string_policy.hpp"

#include <memory>

#include "3party/jansson/myjansson.hpp"
#include "3party/boost/boost/range/adaptor/reversed.hpp"

namespace generator
{
namespace regions
{
std::string JsonPolicy::ToString(Node::PtrList const & nodePtrList)
{
  auto const & main = nodePtrList.front()->GetData();
  auto const & country = nodePtrList.back()->GetData();

  auto geometry = base::NewJSONObject();
  ToJSONObject(*geometry, "type", "Point");
  auto coordinates = base::NewJSONArray();
  auto const center = main.GetCenter();
  ToJSONArray(*coordinates, center.get<0>());
  ToJSONArray(*coordinates, center.get<1>());
  ToJSONObject(*geometry, "coordinates", coordinates);

  auto localeEn = base::NewJSONObject();
  auto address = base::NewJSONObject();
  for (auto const & p : boost::adaptors::reverse(nodePtrList))
  {

    auto const & region = p->GetData();
    auto const label = region.GetLabel();
    ToJSONObject(*address, label, region.GetName());
    if (m_extendedOutput)
    {
      ToJSONObject(*address, label + "_i", region.GetId().GetSerialId());
      ToJSONObject(*address, label + "_a", region.GetArea());
      ToJSONObject(*address, label + "_r", region.GetRank());
    }

    ToJSONObject(*localeEn, label, region.GetEnglishOrTransliteratedName());
  }

  auto locales = base::NewJSONObject();
  ToJSONObject(*locales, "en", localeEn);

  auto properties = base::NewJSONObject();
  ToJSONObject(*properties, "name", main.GetName());
  ToJSONObject(*properties, "rank", main.GetRank());
  ToJSONObject(*properties, "address", address);
  ToJSONObject(*properties, "locales", locales);
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

#include "generator/brands_loader.hpp"

#include "indexer/brands_holder.hpp"

#include "platform/platform.hpp"

#include "base/exception.hpp"
#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include <glaze/json.hpp>

#include <algorithm>
#include <cstdint>
#include <optional>
#include <set>
#include <unordered_map>
#include <utility>
#include <vector>

namespace generator
{
using base::GeoObjectId;
using std::pair, std::string, std::unordered_map, std::vector;

namespace brands_json
{
using FeatureToBrand = std::unordered_map<std::string, uint32_t>;

struct Brands
{
  FeatureToBrand nodes;
  FeatureToBrand ways;
  FeatureToBrand relations;
};

struct Translation
{
  std::optional<std::vector<std::string>> en;
};

using Translations = std::unordered_map<std::string, Translation>;
}  // namespace brands_json

DECLARE_EXCEPTION(ParsingError, RootException);

static bool ReadJson(string const & jsonBuffer, char const * filename, auto & result)
{
  glz::opts constexpr opts{.error_on_unknown_keys = false, .error_on_missing_keys = false};
  if (auto const error = glz::read<opts>(result, jsonBuffer); error)
  {
    LOG(LERROR, ("Cannot parse", filename, glz::format_error(error, jsonBuffer)));
    return false;
  }

  return true;
}

static void ParseFeatureToBrand(brands_json::FeatureToBrand const & featureToBrand, GeoObjectId::Type type,
                                vector<pair<GeoObjectId, uint32_t>> & result)
{
  for (auto const & [key, brandId] : featureToBrand)
  {
    result.emplace_back();
    uint64_t id;
    if (!strings::to_uint64(key, id))
      MYTHROW(ParsingError, ("Incorrect OSM id:", key));
    result.back().first = GeoObjectId(type, id);
    result.back().second = brandId;
  }
}

void ParseTranslations(brands_json::Translations const & translations, std::set<string> const & keys,
                       unordered_map<uint32_t, string> & idToKey)
{
  string const empty;
  auto getKey = [&](string & translation) -> string const &
  {
    strings::MakeLowerCaseInplace(translation);
    translation = strings::Normalize(translation);
    replace(translation.begin(), translation.end(), ' ', '_');
    replace(translation.begin(), translation.end(), '-', '_');
    replace(translation.begin(), translation.end(), '\'', '_');
    auto const it = keys.find(translation);
    if (it != keys.end())
      return *it;
    return empty;
  };

  for (auto const & [key, value] : translations)
  {
    if (!value.en)
      continue;

    for (auto translation : *value.en)
    {
      auto const & indexKey = getKey(translation);
      if (!indexKey.empty())
      {
        uint32_t id;
        if (!strings::to_uint(key, id))
          MYTHROW(ParsingError, ("Incorrect brand id:", key));
        idToKey.emplace(id, indexKey);
        break;
      }
    }
  }
}

bool LoadBrands(string const & brandsFilename, string const & translationsFilename,
                unordered_map<GeoObjectId, string> & brands)
{
  string jsonBuffer;
  try
  {
    GetPlatform().GetReader(brandsFilename)->ReadAsString(jsonBuffer);
  }
  catch (RootException const & e)
  {
    LOG(LERROR, ("Can't open", brandsFilename, e.Msg()));
    return false;
  }

  vector<pair<GeoObjectId, uint32_t>> objects;
  try
  {
    brands_json::Brands parsedBrands;
    if (!ReadJson(jsonBuffer, brandsFilename.c_str(), parsedBrands))
      return false;

    ParseFeatureToBrand(parsedBrands.nodes, GeoObjectId::Type::ObsoleteOsmNode, objects);
    ParseFeatureToBrand(parsedBrands.ways, GeoObjectId::Type::ObsoleteOsmWay, objects);
    ParseFeatureToBrand(parsedBrands.relations, GeoObjectId::Type::ObsoleteOsmRelation, objects);
  }
  catch (ParsingError const & e)
  {
    LOG(LERROR, ("Invalid data in", brandsFilename, e.Msg()));
    return false;
  }

  try
  {
    GetPlatform().GetReader(translationsFilename)->ReadAsString(jsonBuffer);
  }
  catch (RootException const & e)
  {
    LOG(LERROR, ("Can't open", translationsFilename, e.Msg()));
    return false;
  }

  unordered_map<uint32_t, string> idToKey;
  try
  {
    brands_json::Translations translations;
    if (!ReadJson(jsonBuffer, translationsFilename.c_str(), translations))
      return false;

    auto const & keys = indexer::GetDefaultBrands().GetKeys();
    ParseTranslations(translations, keys, idToKey);
  }
  catch (ParsingError const & e)
  {
    LOG(LERROR, ("Invalid data in", translationsFilename, e.Msg()));
    return false;
  }

  for (auto const & o : objects)
  {
    auto const keyIt = idToKey.find(o.second);
    if (keyIt != idToKey.end())
      brands.emplace(o.first, keyIt->second);
  }

  LOG(LINFO, (idToKey.size(), "brands for", brands.size(), "objects successfully loaded.",
              objects.size() - brands.size(), "objects not matched to known brands."));
  return true;
}
}  // namespace generator

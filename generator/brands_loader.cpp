#include "generator/brands_loader.hpp"

#include "indexer/brands_holder.hpp"

#include "platform/platform.hpp"

#include "base/exception.hpp"
#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include "3party/jansson/myjansson.hpp"

#include <algorithm>
#include <cstdint>
#include <utility>
#include <vector>

using namespace std;
using base::GeoObjectId;

namespace
{
DECLARE_EXCEPTION(ParsingError, RootException);

void ParseFeatureToBrand(json_t * root, string const & field, GeoObjectId::Type type,
                         vector<pair<GeoObjectId, uint32_t>> & result)
{
  auto arr = base::GetJSONOptionalField(root, field);
  if (arr == nullptr)
    return;

  char const * key;
  json_t * value;
  json_object_foreach(arr, key, value)
  {
    result.push_back(pair<GeoObjectId, uint32_t>());
    uint64_t id;
    if (!strings::to_uint64(key, id))
      MYTHROW(ParsingError, ("Incorrect OSM id:", key));
    result.back().first = GeoObjectId(type, id);
    FromJSON(value, result.back().second);
  }
}

void ParseTranslations(json_t * root, set<string> const & keys,
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

  char const * key;
  json_t * value;
  json_object_foreach(root, key, value)
  {
    vector<string> enTranslations;
    if (!FromJSONObjectOptional(value, "en", enTranslations))
      continue;

    for (auto & translation : enTranslations)
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
}  // namespace

namespace generator
{
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
    base::Json root(jsonBuffer.c_str());
    CHECK(root.get() != nullptr, ("Cannot parse the json file:", brandsFilename));

    ParseFeatureToBrand(root.get(), "nodes", GeoObjectId::Type::ObsoleteOsmNode, objects);
    ParseFeatureToBrand(root.get(), "ways", GeoObjectId::Type::ObsoleteOsmWay, objects);
    ParseFeatureToBrand(root.get(), "relations", GeoObjectId::Type::ObsoleteOsmRelation, objects);
  }
  catch (base::Json::Exception const &)
  {
    LOG(LERROR, ("Cannot create base::Json from", brandsFilename));
    return false;
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
    base::Json root(jsonBuffer.c_str());
    CHECK(root.get() != nullptr, ("Cannot parse the json file:", translationsFilename));
    auto const & keys = indexer::GetDefaultBrands().GetKeys();
    ParseTranslations(root.get(), keys, idToKey);
  }
  catch (base::Json::Exception const &)
  {
    LOG(LERROR, ("Cannot create base::Json from", translationsFilename));
    return false;
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

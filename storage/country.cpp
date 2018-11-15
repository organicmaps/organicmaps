#include "storage/country.hpp"

#include "platform/mwm_version.hpp"
#include "platform/platform.hpp"

#include "coding/reader.hpp"

#include "base/logging.hpp"
#include "base/stl_helpers.hpp"

#include "3party/jansson/myjansson.hpp"

#include <utility>

using namespace std;
using platform::CountryFile;

namespace storage
{
// Mwm subtree attributes. They can be calculated based on information contained in countries.txt.
// The first in the pair is number of mwms in a subtree. The second is sum of sizes of
// all mwms in a subtree.
using TMwmSubtreeAttrs = pair<TMwmCounter, TMwmSize>;

namespace
{
class StoreSingleMwmInterface
{
public:
  virtual ~StoreSingleMwmInterface() = default;
  virtual Country * InsertToCountryTree(TCountryId const & id, TMwmSize mapSize,
                                        string const & mapSha1, size_t depth,
                                        TCountryId const & parent) = 0;
  virtual void InsertOldMwmMapping(TCountryId const & newId, TCountryId const & oldId) = 0;
  virtual void InsertAffiliation(TCountryId const & countryId, string const & affilation) = 0;
  virtual TMappingOldMwm GetMapping() const = 0;
};

class StoreCountriesSingleMwms : public StoreSingleMwmInterface
{
  TCountryTree & m_countries;
  TMappingAffiliations & m_affiliations;
  TMappingOldMwm m_idsMapping;

public:
  StoreCountriesSingleMwms(TCountryTree & countries, TMappingAffiliations & affiliations)
    : m_countries(countries), m_affiliations(affiliations)
  {
  }
  ~StoreCountriesSingleMwms()
  {
    for (auto & entry : m_affiliations)
      base::SortUnique(entry.second);
  }

  // StoreSingleMwmInterface overrides:
  Country * InsertToCountryTree(TCountryId const & id, TMwmSize mapSize, string const & mapSha1,
                                size_t depth, TCountryId const & parent) override
  {
    Country country(id, parent);
    if (mapSize)
    {
      CountryFile countryFile(id);
      countryFile.SetRemoteSizes(mapSize, 0 /* routingSize */);
      countryFile.SetSha1(mapSha1);
      country.SetFile(countryFile);
    }
    return &m_countries.AddAtDepth(depth, country);
  }

  void InsertOldMwmMapping(TCountryId const & newId, TCountryId const & oldId) override
  {
    m_idsMapping[oldId].insert(newId);
  }

  void InsertAffiliation(TCountryId const & countryId, string const & affilation) override
  {
    ASSERT(!affilation.empty(), ());
    ASSERT(!countryId.empty(), ());

    m_affiliations[affilation].push_back(countryId);
  }

  TMappingOldMwm GetMapping() const override { return m_idsMapping; }
};

class StoreFile2InfoSingleMwms : public StoreSingleMwmInterface
{
  TMappingOldMwm m_idsMapping;
  map<string, CountryInfo> & m_file2info;

public:
  StoreFile2InfoSingleMwms(map<string, CountryInfo> & file2info) : m_file2info(file2info) {}
  // StoreSingleMwmInterface overrides:
  Country * InsertToCountryTree(TCountryId const & id, TMwmSize /* mapSize */,
                                string const & /* mapSha1 */, size_t /* depth */,
                                TCountryId const & /* parent */) override
  {
    CountryInfo info(id);
    m_file2info[id] = move(info);
    return nullptr;
  }

  void InsertOldMwmMapping(TCountryId const & /* newId */, TCountryId const & /* oldId */) override
  {
  }
  void InsertAffiliation(TCountryId const & /* countryId */,
                         string const & /* affilation */) override
  {
  }
  TMappingOldMwm GetMapping() const override
  {
    ASSERT(false, ());
    return map<TCountryId, TCountriesSet>();
  }
};
}  // namespace

TMwmSubtreeAttrs LoadGroupSingleMwmsImpl(size_t depth, json_t * node, TCountryId const & parent,
                                         StoreSingleMwmInterface & store)
{
  TCountryId id;
  FromJSONObject(node, "id", id);

  // Mapping two component (big) mwms to one componenst (small) ones.
  vector<string> oldIds;
  FromJSONObjectOptionalField(node, "old", oldIds);
  for (auto const & oldId : oldIds)
    store.InsertOldMwmMapping(id, oldId);

  // Mapping affiliations to one component (small) mwms.
  vector<string> affiliations;
  FromJSONObjectOptionalField(node, "affiliations", affiliations);
  for (auto const & affilationValue : affiliations)
    store.InsertAffiliation(id, affilationValue);

  int nodeSize;
  FromJSONObjectOptionalField(node, "s", nodeSize);
  ASSERT_LESS_OR_EQUAL(0, nodeSize, ());

  string nodeHash;
  FromJSONObjectOptionalField(node, "sha1_base64", nodeHash);

  // We expect that mwm and routing files should be less than 2GB.
  Country * addedNode = store.InsertToCountryTree(id, nodeSize, nodeHash, depth, parent);

  TMwmCounter mwmCounter = 0;
  TMwmSize mwmSize = 0;
  vector<json_t *> children;
  FromJSONObjectOptionalField(node, "g", children);
  if (children.empty())
  {
    mwmCounter = 1;  // It's a leaf. Any leaf contains one mwm.
    mwmSize = nodeSize;
  }
  else
  {
    for (json_t * child : children)
    {
      TMwmSubtreeAttrs const childAttr = LoadGroupSingleMwmsImpl(depth + 1, child, id, store);
      mwmCounter += childAttr.first;
      mwmSize += childAttr.second;
    }
  }

  if (addedNode != nullptr)
    addedNode->SetSubtreeAttrs(mwmCounter, mwmSize);

  return make_pair(mwmCounter, mwmSize);
}

bool LoadCountriesSingleMwmsImpl(string const & jsonBuffer, StoreSingleMwmInterface & store)
{
  try
  {
    base::Json root(jsonBuffer.c_str());
    LoadGroupSingleMwmsImpl(0 /* depth */, root.get(), kInvalidCountryId, store);
    return true;
  }
  catch (base::Json::Exception const & e)
  {
    LOG(LERROR, (e.Msg()));
    return false;
  }
}

namespace
{
class StoreTwoComponentMwmInterface
{
public:
  virtual ~StoreTwoComponentMwmInterface() = default;
  virtual Country * Insert(string const & id, TMwmSize mapSize, TMwmSize /* routingSize */,
                           size_t depth, TCountryId const & parent) = 0;
};

class StoreCountriesTwoComponentMwms : public StoreTwoComponentMwmInterface
{
  TCountryTree & m_countries;

public:
  StoreCountriesTwoComponentMwms(TCountryTree & countries,
                                 TMappingAffiliations & /* affiliations */)
    : m_countries(countries)
  {
  }

  // StoreTwoComponentMwmInterface overrides:
  virtual Country * Insert(string const & id, TMwmSize mapSize, TMwmSize routingSize, size_t depth,
                           TCountryId const & parent) override
  {
    Country country(id, parent);
    if (mapSize)
    {
      CountryFile countryFile(id);
      countryFile.SetRemoteSizes(mapSize, routingSize);
      country.SetFile(countryFile);
    }
    return &m_countries.AddAtDepth(depth, country);
  }
};

class StoreFile2InfoTwoComponentMwms : public StoreTwoComponentMwmInterface
{
  TMappingOldMwm m_idsMapping;
  map<string, CountryInfo> & m_file2info;

public:
  StoreFile2InfoTwoComponentMwms(map<string, CountryInfo> & file2info) : m_file2info(file2info) {}
  // StoreTwoComponentMwmInterface overrides:
  virtual Country * Insert(string const & id, TMwmSize mapSize, TMwmSize /* routingSize */,
                           size_t /* depth */, TCountryId const & /* parent */) override
  {
    if (mapSize == 0)
      return nullptr;

    CountryInfo info(id);
    m_file2info[id] = info;
    return nullptr;
  }
};
}  // namespace

TMwmSubtreeAttrs LoadGroupTwoComponentMwmsImpl(size_t depth, json_t * node,
                                               TCountryId const & parent,
                                               StoreTwoComponentMwmInterface & store)
{
  // @TODO(bykoianko) After we stop supporting two component mwms (with routing files)
  // remove code below.
  TCountryId file;
  FromJSONObjectOptionalField(node, "f", file);
  if (file.empty())
    FromJSONObject(node, "n", file);  // If file is empty, it's the same as the name.

  // We expect that mwm and routing files should be less than 2GB.
  int mwmSize, routingSize;
  FromJSONObjectOptionalField(node, "s", mwmSize);
  FromJSONObjectOptionalField(node, "rs", routingSize);
  ASSERT_LESS_OR_EQUAL(0, mwmSize, ());
  ASSERT_LESS_OR_EQUAL(0, routingSize, ());

  Country * addedNode = store.Insert(file, static_cast<TMwmSize>(mwmSize),
                                     static_cast<TMwmSize>(routingSize), depth, parent);

  TMwmCounter countryCounter = 0;
  TMwmSize countrySize = 0;
  vector<json_t *> children;
  FromJSONObjectOptionalField(node, "g", children);
  if (children.empty())
  {
    countryCounter = 1;  // It's a leaf. Any leaf contains one mwm.
    countrySize = mwmSize + routingSize;
  }
  else
  {
    for (json_t * child : children)
    {
      TMwmSubtreeAttrs const childAttr =
          LoadGroupTwoComponentMwmsImpl(depth + 1, child, file, store);
      countryCounter += childAttr.first;
      countrySize += childAttr.second;
    }
  }

  if (addedNode != nullptr)
    addedNode->SetSubtreeAttrs(countryCounter, countrySize);

  return make_pair(countryCounter, countrySize);
}

bool LoadCountriesTwoComponentMwmsImpl(string const & jsonBuffer,
                                       StoreTwoComponentMwmInterface & store)
{
  try
  {
    base::Json root(jsonBuffer.c_str());
    LoadGroupTwoComponentMwmsImpl(0 /* depth */, root.get(), kInvalidCountryId, store);
    return true;
  }
  catch (base::Json::Exception const & e)
  {
    LOG(LERROR, (e.Msg()));
    return false;
  }
}

int64_t LoadCountriesFromBuffer(string const & jsonBuffer, TCountryTree & countries,
                                TMappingAffiliations & affiliations,
                                TMappingOldMwm * mapping /* = nullptr */)
{
  countries.Clear();
  affiliations.clear();

  int64_t version = -1;
  try
  {
    base::Json root(jsonBuffer.c_str());
    FromJSONObject(root.get(), "v", version);

    if (version::IsSingleMwm(version))
    {
      StoreCountriesSingleMwms store(countries, affiliations);
      if (!LoadCountriesSingleMwmsImpl(jsonBuffer, store))
        return -1;
      if (mapping)
        *mapping = store.GetMapping();
    }
    else
    {
      StoreCountriesTwoComponentMwms store(countries, affiliations);
      if (!LoadCountriesTwoComponentMwmsImpl(jsonBuffer, store))
        return -1;
    }
  }
  catch (base::Json::Exception const & e)
  {
    LOG(LERROR, (e.Msg()));
  }
  return version;
}

int64_t LoadCountriesFromFile(string const & path, TCountryTree & countries,
                              TMappingAffiliations & affiliations, TMappingOldMwm * mapping)
{
  string json;
  ReaderPtr<Reader>(GetPlatform().GetReader(path)).ReadAsString(json);
  return LoadCountriesFromBuffer(json, countries, affiliations, mapping);
}

void LoadCountryFile2CountryInfo(string const & jsonBuffer, map<string, CountryInfo> & id2info,
                                 bool & isSingleMwm)
{
  ASSERT(id2info.empty(), ());

  int64_t version = -1;
  try
  {
    base::Json root(jsonBuffer.c_str());
    FromJSONObjectOptionalField(root.get(), "v", version);
    isSingleMwm = version::IsSingleMwm(version);
    if (isSingleMwm)
    {
      StoreFile2InfoSingleMwms store(id2info);
      LoadCountriesSingleMwmsImpl(jsonBuffer, store);
    }
    else
    {
      StoreFile2InfoTwoComponentMwms store(id2info);
      LoadCountriesTwoComponentMwmsImpl(jsonBuffer, store);
    }
  }
  catch (base::Json::Exception const & e)
  {
    LOG(LERROR, (e.Msg()));
  }
}
}  // namespace storage

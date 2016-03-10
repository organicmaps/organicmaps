#include "storage/country.hpp"

#include "platform/mwm_version.hpp"
#include "platform/platform.hpp"

#include "base/logging.hpp"

#include "3party/jansson/myjansson.hpp"

#include "std/utility.hpp"

using platform::CountryFile;

namespace storage
{
// Mwm subtree attributes. They can be calculated based on information contained in countries.txt.
// The first in the pair is number of mwms in a subtree. The second is sum of sizes of
// all mwms in a subtree.
using TMwmSubtreeAttrs = pair<uint32_t, size_t>;

namespace
{
class StoreSingleMwmInterface
{
public:
  virtual Country * InsertToCountryTree(TCountryId const & id, uint32_t mapSize, int depth,
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
    : m_countries(countries), m_affiliations(affiliations) {}

  Country * InsertToCountryTree(TCountryId const & id, uint32_t mapSize, int depth,
                                TCountryId const & parent) override
  {
    Country country(id, parent);
    if (mapSize)
    {
      CountryFile countryFile(id);
      countryFile.SetRemoteSizes(mapSize, 0 /* routingSize */);
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
    m_affiliations.insert(make_pair(countryId, affilation));
  }

  TMappingOldMwm GetMapping() const override { return m_idsMapping; }
};

class StoreFile2InfoSingleMwms : public StoreSingleMwmInterface
{
  TMappingOldMwm m_idsMapping;
  map<string, CountryInfo> & m_file2info;

public:
  StoreFile2InfoSingleMwms(map<string, CountryInfo> & file2info)
    : m_file2info(file2info) {}

  Country * InsertToCountryTree(TCountryId const & id, uint32_t /* mapSize */, int /* depth */,
                                TCountryId const & /* parent */) override
  {
    CountryInfo info(id);
    m_file2info[id] = move(info);
    return nullptr;
  }

  void InsertOldMwmMapping(TCountryId const & /* newId */, TCountryId const & /* oldId */) override {}
  void InsertAffiliation(TCountryId const & /* countryId */, string const & /* affilation */) override {}
  TMappingOldMwm GetMapping() const override
  {
    ASSERT(false, ());
    return map<TCountryId, TCountriesSet>();
  }
};
}  // namespace

TMwmSubtreeAttrs LoadGroupSingleMwmsImpl(int depth, json_t * node, TCountryId const & parent,
                                         StoreSingleMwmInterface & store)
{
  uint32_t mwmCounter = 0;
  size_t mwmSize = 0;

  char const * id = json_string_value(json_object_get(node, "id"));
  if (!id)
    MYTHROW(my::Json::Exception, ("LoadGroupImpl. Id is missing.", id));

  // Mapping two component (big) mwms to one componenst (small) ones.
  json_t * oldIds = json_object_get(node, "old");
  if (oldIds)
  {
    size_t const oldListSize = json_array_size(oldIds);
    for (size_t k = 0; k < oldListSize; ++k)
    {
      string oldIdValue = json_string_value(json_array_get(oldIds, k));
      store.InsertOldMwmMapping(id, oldIdValue);
    }
  }

  // Mapping affiliations to one componenst (small) mwms.
  json_t * affiliations = json_object_get(node, "affiliations");
  if (affiliations)
  {
    size_t const affiliationsSize = json_array_size(affiliations);
    for (size_t k = 0; k < affiliationsSize; ++k)
    {
      string affilationValue = json_string_value(json_array_get(affiliations, k));
      store.InsertAffiliation(id, affilationValue);
    }
  }

  uint32_t const nodeSize = static_cast<uint32_t>(json_integer_value(json_object_get(node, "s")));
  // We expect that mwm and routing files should be less than 2GB.
  Country * addedNode = store.InsertToCountryTree(id, nodeSize, depth, parent);

  json_t * children = json_object_get(node, "g");
  if (children)
  {
    size_t const groupListSize = json_array_size(children);
    for (size_t i = 0; i < groupListSize; ++i)
    {
      json_t * j = json_array_get(children, i);
      TMwmSubtreeAttrs const childAttr = LoadGroupSingleMwmsImpl(depth + 1, j, id, store);
      mwmCounter += childAttr.first;
      mwmSize += childAttr.second;
    }
  }
  else
  {
    mwmCounter = 1; // It's a leaf. Any leaf contains one mwm.
    mwmSize = nodeSize;
  }

  if (addedNode != nullptr)
    addedNode->SetSubtreeAttrs(mwmCounter, mwmSize);

  return make_pair(mwmCounter, mwmSize);
}

bool LoadCountriesSingleMwmsImpl(string const & jsonBuffer, StoreSingleMwmInterface & store)
{
  try
  {
    my::Json root(jsonBuffer.c_str());
    LoadGroupSingleMwmsImpl(0 /* depth */, root.get(), kInvalidCountryId, store);
    return true;
  }
  catch (my::Json::Exception const & e)
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
  virtual void Insert(string const & id, uint32_t mapSize, uint32_t /* routingSize */,
                      int depth, TCountryId const & parent) = 0;
};

class StoreCountriesTwoComponentMwms : public StoreTwoComponentMwmInterface
{
  TCountryTree & m_countries;

public:
  StoreCountriesTwoComponentMwms(TCountryTree & countries, TMappingAffiliations & /* affiliations */)
    : m_countries(countries) {}

  void Insert(string const & id, uint32_t mapSize, uint32_t routingSize,
              int depth, TCountryId const & parent) override
  {
    Country country(id, parent);
    if (mapSize)
    {
      CountryFile countryFile(id);
      countryFile.SetRemoteSizes(mapSize, routingSize);
      country.SetFile(countryFile);
    }
    m_countries.AddAtDepth(depth, country);
  }
};

class StoreFile2InfoTwoComponentMwms : public StoreTwoComponentMwmInterface
{
  TMappingOldMwm m_idsMapping;
  map<string, CountryInfo> & m_file2info;

public:
  StoreFile2InfoTwoComponentMwms(map<string, CountryInfo> & file2info)
    : m_file2info(file2info) {}

  void Insert(string const & id, uint32_t mapSize, uint32_t /* routingSize */,
              int /* depth */, TCountryId const & /* parent */) override
  {
    if (mapSize == 0)
      return;

    CountryInfo info(id);
    m_file2info[id] = info;
  }
};
}  // namespace

void LoadGroupTwoComponentMwmsImpl(int depth, json_t * node, TCountryId const & parent,
                                   StoreTwoComponentMwmInterface & store)
{
  // @TODO(bykoianko) After we stop supporting two component mwms (with routing files)
  // remove code below.
  char const * file = json_string_value(json_object_get(node, "f"));
  // If file is empty, it's the same as the name.
  if (!file)
  {
    file = json_string_value(json_object_get(node, "n"));
    if (!file)
      MYTHROW(my::Json::Exception, ("Country name is missing"));
  }

  // We expect that mwm and routing files should be less than 2GB.
  uint32_t const mwmSize = static_cast<uint32_t>(json_integer_value(json_object_get(node, "s")));
  uint32_t const routingSize = static_cast<uint32_t>(json_integer_value(json_object_get(node, "rs")));
  store.Insert(file, mwmSize, routingSize, depth, parent);

  json_t * children = json_object_get(node, "g");
  if (children)
  {
    size_t const groupListSize = json_array_size(children);
    for (size_t i = 0; i < groupListSize; ++i)
    {
      json_t * j = json_array_get(children, i);
      LoadGroupTwoComponentMwmsImpl(depth + 1, j, file, store);
    }
  }
}

bool LoadCountriesTwoComponentMwmsImpl(string const & jsonBuffer,
                                       StoreTwoComponentMwmInterface & store)
{
  try
  {
    my::Json root(jsonBuffer.c_str());
    LoadGroupTwoComponentMwmsImpl(0 /* depth */, root.get(), kInvalidCountryId, store);
    return true;
  }
  catch (my::Json::Exception const & e)
  {
    LOG(LERROR, (e.Msg()));
    return false;
  }
}

int64_t LoadCountries(string const & jsonBuffer, TCountryTree & countries,
                      TMappingAffiliations & affiliations, TMappingOldMwm * mapping /* = nullptr */)
{
  countries.Clear();
  affiliations.clear();

  int64_t version = -1;
  try
  {
    my::Json root(jsonBuffer.c_str());
    json_t * const rootPtr = root.get();
    version = json_integer_value(json_object_get(rootPtr, "v"));

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
  catch (my::Json::Exception const & e)
  {
    LOG(LERROR, (e.Msg()));
  }
  return version;
}

void LoadCountryFile2CountryInfo(string const & jsonBuffer, map<string, CountryInfo> & id2info,
                                 bool & isSingleMwm)
{
  ASSERT(id2info.empty(), ());

  int64_t version = -1;
  try
  {
    my::Json root(jsonBuffer.c_str());
    version = json_integer_value(json_object_get(root.get(), "v"));
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
  catch (my::Json::Exception const & e)
  {
    LOG(LERROR, (e.Msg()));
  }
}
}  // namespace storage

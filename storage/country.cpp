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

template <class ToDo>
TMwmSubtreeAttrs LoadGroupSingleMwmsImpl(int depth, json_t * group, TCountryId const & parent, ToDo & toDo)
{
  uint32_t mwmCounter = 0;
  size_t mwmSize = 0;
  size_t const groupListSize = json_array_size(group);
  toDo.ReserveAtDepth(depth, groupListSize);
  for (size_t i = 0; i < groupListSize; ++i)
  {
    json_t * j = json_array_get(group, i);

    char const * id = json_string_value(json_object_get(j, "id"));
    if (!id)
      MYTHROW(my::Json::Exception, ("LoadGroupImpl. Id is missing.", id));

    uint32_t const nodeSize = static_cast<uint32_t>(json_integer_value(json_object_get(j, "s")));
    // We expect that mwm and routing files should be less than 2GB.
    Country * addedNode = toDo(id, nodeSize, depth, parent);

    json_t * oldIds = json_object_get(j, "old");
    if (oldIds)
    {
      size_t const oldListSize = json_array_size(oldIds);
      for (size_t k = 0; k < oldListSize; ++k)
      {
        string oldIdValue = json_string_value(json_array_get(oldIds, k));
        toDo(oldIdValue, id);
      }
    }

    uint32_t mwmChildCounter = 0;
    size_t mwmChildSize = 0;
    json_t * children = json_object_get(j, "g");
    if (children)
    {
      TMwmSubtreeAttrs childAttr = LoadGroupSingleMwmsImpl(depth + 1, children, id, toDo);
      mwmChildCounter = childAttr.first;
      mwmChildSize = childAttr.second;
    }
    else
    {
      mwmChildCounter = 1; // It's a leaf. Any leaf contains one mwm.
      mwmChildSize = nodeSize;
    }
    mwmCounter += mwmChildCounter;
    mwmSize += mwmChildSize;

    if (addedNode != nullptr)
      addedNode->SetSubtreeAttrs(mwmChildCounter, mwmChildSize);
  }
  return make_pair(mwmCounter, mwmSize);
}

template <class ToDo>
void LoadGroupTwoComponentMwmsImpl(int depth, json_t * group, TCountryId const & parent, ToDo & toDo)
{
  // @TODO(bykoianko) After we stop supporting two component mwms (with routing files)
  // remove code below.
  size_t const groupListSize = json_array_size(group);
  toDo.ReserveAtDepth(depth, groupListSize);
  for (size_t i = 0; i < groupListSize; ++i)
  {
    json_t * j = json_array_get(group, i);

    char const * file = json_string_value(json_object_get(j, "f"));
    // if file is empty, it's the same as the name
    if (!file)
    {
      file = json_string_value(json_object_get(j, "n"));
      if (!file)
        MYTHROW(my::Json::Exception, ("Country name is missing"));
    }

    // We expect that mwm and routing files should be less than 2GB.
    uint32_t const mwmSize = static_cast<uint32_t>(json_integer_value(json_object_get(j, "s")));
    uint32_t const routingSize = static_cast<uint32_t>(json_integer_value(json_object_get(j, "rs")));
    toDo(file, mwmSize, routingSize, depth, parent);

    json_t * children = json_object_get(j, "g");
    if (children)
      LoadGroupTwoComponentMwmsImpl(depth + 1, children, file, toDo);
  }
}

template <class ToDo>
bool LoadCountriesSingleMwmsImpl(string const & jsonBuffer,TCountryId const & parent, ToDo & toDo)
{
  try
  {
    my::Json root(jsonBuffer.c_str());
    json_t * children = json_object_get(root.get(), "g");
    if (!children)
      MYTHROW(my::Json::Exception, ("Root country doesn't have any groups"));
    TMwmSubtreeAttrs const treeAttrs = LoadGroupSingleMwmsImpl(0 /* depth */, children, parent, toDo);
    toDo.SetCountriesContainerAttrs(treeAttrs.first /* mwmNumber */,
                                    treeAttrs.second /* mwmSizeBytes */);
    return true;
  }
  catch (my::Json::Exception const & e)
  {
    LOG(LERROR, (e.Msg()));
    return false;
  }
}

template <class ToDo>
bool LoadCountriesTwoComponentMwmsImpl(string const & jsonBuffer, TCountryId const & parent, ToDo & toDo)
{
  try
  {
    my::Json root(jsonBuffer.c_str());
    json_t * children = json_object_get(root.get(), "g");
    if (!children)
      MYTHROW(my::Json::Exception, ("Root country doesn't have any groups"));
    LoadGroupTwoComponentMwmsImpl(0 /* depth */, children, parent, toDo);
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
class DoStoreCountriesSingleMwms
{
  TCountriesContainer & m_cont;
  TMapping m_idsMapping;

public:
  DoStoreCountriesSingleMwms(TCountriesContainer & cont) : m_cont(cont) {}

  Country * operator()(TCountryId const & id, uint32_t mapSize, int depth, TCountryId const & parent)
  {
    Country country(id, parent);
    if (mapSize)
    {
      CountryFile countryFile(id);
      countryFile.SetRemoteSizes(mapSize, 0 /* routingSize */);
      country.SetFile(countryFile);
    }
    return &m_cont.AddAtDepth(depth, country);
  }

  void operator()(TCountryId const & oldId, TCountryId const & newId)
  {
    m_idsMapping[oldId].insert(newId);
  }

  void SetCountriesContainerAttrs(uint32_t mwmNumber, size_t mwmSizeBytes)
  {
    m_cont.Value().SetSubtreeAttrs(mwmNumber, mwmSizeBytes);
  }

  TMapping GetMapping() const { return m_idsMapping; }

  void ReserveAtDepth(int level, size_t n) { m_cont.ReserveAtDepth(level, n); }
};

class DoStoreCountriesTwoComponentMwms
{
  TCountriesContainer & m_cont;

public:
  DoStoreCountriesTwoComponentMwms(TCountriesContainer & cont) : m_cont(cont) {}

  void operator()(string const & file, uint32_t mapSize,
                  uint32_t routingSize, int depth, TCountryId const & parent)
  {
    Country country(file, parent);
    if (mapSize)
    {
      CountryFile countryFile(file);
      countryFile.SetRemoteSizes(mapSize, routingSize);
      country.SetFile(countryFile);
    }
    m_cont.AddAtDepth(depth, country);
  }

  void ReserveAtDepth(int level, size_t n) { m_cont.ReserveAtDepth(level, n); }
};

class DoStoreFile2InfoSingleMwms
{
  TMapping m_idsMapping;
  map<string, CountryInfo> & m_file2info;

public:
  DoStoreFile2InfoSingleMwms(map<string, CountryInfo> & file2info)
    : m_file2info(file2info) {}

  Country * operator()(TCountryId const & id, uint32_t /* mapSize */, int /* depth */,
                       TCountryId const & /* parent */)
  {
    CountryInfo info(id);
    m_file2info[id] = move(info);
    return nullptr;
  }

  void operator()(TCountryId const & oldId, TCountryId const & newId)
  {
    m_idsMapping[oldId].insert(newId);
  }

  void SetCountriesContainerAttrs(uint32_t, size_t) {}
  TMapping GetMapping() const { return m_idsMapping; }
  void ReserveAtDepth(int, size_t) {}
};

class DoStoreFile2InfoTwoComponentMwms
{
  TMapping m_idsMapping;
  map<string, CountryInfo> & m_file2info;

public:
  DoStoreFile2InfoTwoComponentMwms(map<string, CountryInfo> & file2info)
    : m_file2info(file2info) {}

  void operator()(string const & id, uint32_t mapSize, uint32_t /* routingSize */, int /* depth */,
                  TCountryId const & /* parent */)
  {
    if (mapSize == 0)
      return;

    CountryInfo info(id);
    m_file2info[id] = info;
  }
  void ReserveAtDepth(int, size_t) {}
};
}  // namespace

int64_t LoadCountries(string const & jsonBuffer, TCountriesContainer & countries, TMapping * mapping /* = nullptr */)
{
  countries.Clear();

  int64_t version = -1;
  try
  {
    my::Json root(jsonBuffer.c_str());
    json_t * const rootPtr = root.get();
    version = json_integer_value(json_object_get(rootPtr, "v"));

    // Extracting root id.
    bool const isSingleMwm = version::IsSingleMwm(version);
    char const * const idKey = isSingleMwm ? "id" : "n";
    char const * id = json_string_value(json_object_get(rootPtr, idKey));
    if (!id)
      MYTHROW(my::Json::Exception, ("LoadCountries. Id is missing.", id));
    Country rootCountry(id, kInvalidCountryId);
    // @TODO(bykoianko) Add CourtyFile to rootCountry with correct size.
    countries.Value() = rootCountry;

    if (isSingleMwm)
    {
      DoStoreCountriesSingleMwms doStore(countries);
      if (!LoadCountriesSingleMwmsImpl(jsonBuffer, id, doStore))
        return -1;
      if (mapping)
        *mapping = doStore.GetMapping();
    }
    else
    {
      DoStoreCountriesTwoComponentMwms doStore(countries);
      if (!LoadCountriesTwoComponentMwmsImpl(jsonBuffer, id, doStore))
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
      DoStoreFile2InfoSingleMwms doStore(id2info);
      LoadCountriesSingleMwmsImpl(jsonBuffer, kInvalidCountryId, doStore);
    }
    else
    {
      DoStoreFile2InfoTwoComponentMwms doStore(id2info);
      LoadCountriesTwoComponentMwmsImpl(jsonBuffer, kInvalidCountryId, doStore);
    }
  }
  catch (my::Json::Exception const & e)
  {
    LOG(LERROR, (e.Msg()));
  }
}
}  // namespace storage

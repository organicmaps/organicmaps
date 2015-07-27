#include "storage/country.hpp"

#include "platform/platform.hpp"

#include "base/logging.hpp"

#include "3party/jansson/myjansson.hpp"

using platform::CountryFile;

namespace storage
{
uint64_t Country::Size(MapOptions opt) const
{
  uint64_t size = 0;
  for (CountryFile const & file : m_files)
    size += file.GetRemoteSize(opt);
  return size;
}

void Country::AddFile(CountryFile const & file) { m_files.push_back(file); }

////////////////////////////////////////////////////////////////////////

template <class ToDo>
void LoadGroupImpl(int depth, json_t * group, ToDo & toDo)
{
  for (size_t i = 0; i < json_array_size(group); ++i)
  {
    json_t * j = json_array_get(group, i);

    // name is mandatory
    char const * name = json_string_value(json_object_get(j, "n"));
    if (!name)
      MYTHROW(my::Json::Exception, ("Country name is missing"));

    char const * file = json_string_value(json_object_get(j, "f"));
    // if file is empty, it's the same as the name
    if (!file)
      file = name;

    char const * flag = json_string_value(json_object_get(j, "c"));
    toDo(name, file, flag ? flag : "",
         // We expect what mwm and routing files should be less 2Gb
         static_cast<uint32_t>(json_integer_value(json_object_get(j, "s"))),
         static_cast<uint32_t>(json_integer_value(json_object_get(j, "rs"))), depth);

    json_t * children = json_object_get(j, "g");
    if (children)
      LoadGroupImpl(depth + 1, children, toDo);
  }
}

template <class ToDo>
bool LoadCountriesImpl(string const & jsonBuffer, ToDo & toDo)
{
  try
  {
    my::Json root(jsonBuffer.c_str());
    json_t * children = json_object_get(root.get(), "g");
    if (!children)
      MYTHROW(my::Json::Exception, ("Root country doesn't have any groups"));
    LoadGroupImpl(0, children, toDo);
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
class DoStoreCountries
{
  CountriesContainerT & m_cont;

public:
  DoStoreCountries(CountriesContainerT & cont) : m_cont(cont) {}

  void operator()(string const & name, string const & file, string const & flag, uint32_t mapSize,
                  uint32_t routingSize, int depth)
  {
    Country country(name, flag);
    if (mapSize)
    {
      CountryFile countryFile(file);
      countryFile.SetRemoteSizes(mapSize, routingSize);
      country.AddFile(countryFile);
    }
    m_cont.AddAtDepth(depth, country);
  }
};

class DoStoreFile2Info
{
  map<string, CountryInfo> & m_file2info;
  string m_lastFlag;

public:
  DoStoreFile2Info(map<string, CountryInfo> & file2info) : m_file2info(file2info) {}

  void operator()(string name, string file, string const & flag, uint32_t mapSize, uint32_t, int)
  {
    if (!flag.empty())
      m_lastFlag = flag;

    if (mapSize)
    {
      CountryInfo info;

      // if 'file' is empty - it's equal to 'name'
      if (!file.empty())
      {
        // make compound name: country_region
        size_t const i = file.find_first_of('_');
        if (i != string::npos)
          name = file.substr(0, i) + '_' + name;

        // fill 'name' only when it differs with 'file'
        if (name != file)
          info.m_name.swap(name);
      }
      else
        file.swap(name);

      // Do not use 'name' here! It was swapped!

      ASSERT(!m_lastFlag.empty(), ());
      info.m_flag = m_lastFlag;

      m_file2info[file] = info;
    }
  }
};

class DoStoreCode2File
{
  multimap<string, string> & m_code2file;

public:
  DoStoreCode2File(multimap<string, string> & code2file) : m_code2file(code2file) {}

  void operator()(string const &, string const & file, string const & flag, uint32_t, uint32_t, int)
  {
    m_code2file.insert(make_pair(flag, file));
  }
};
}

int64_t LoadCountries(string const & jsonBuffer, CountriesContainerT & countries)
{
  countries.Clear();

  int64_t version = -1;
  try
  {
    my::Json root(jsonBuffer.c_str());
    version = json_integer_value(json_object_get(root.get(), "v"));
    DoStoreCountries doStore(countries);
    if (!LoadCountriesImpl(jsonBuffer, doStore))
      return -1;
  }
  catch (my::Json::Exception const & e)
  {
    LOG(LERROR, (e.Msg()));
  }
  return version;
}

void LoadCountryFile2CountryInfo(string const & jsonBuffer, map<string, CountryInfo> & id2info)
{
  ASSERT(id2info.empty(), ());
  DoStoreFile2Info doStore(id2info);
  LoadCountriesImpl(jsonBuffer, doStore);
}

void LoadCountryCode2File(string const & jsonBuffer, multimap<string, string> & code2file)
{
  ASSERT(code2file.empty(), ());
  DoStoreCode2File doStore(code2file);
  LoadCountriesImpl(jsonBuffer, doStore);
}

template <class T>
void SaveImpl(T const & v, json_t * jParent)
{
  size_t const siblingsCount = v.SiblingsCount();
  CHECK_GREATER(siblingsCount, 0, ());

  my::JsonHandle jArray;
  jArray.AttachNew(json_array());
  for (size_t i = 0; i < siblingsCount; ++i)
  {
    my::JsonHandle jCountry;
    jCountry.AttachNew(json_object());

    string const strName = v[i].Value().Name();
    CHECK(!strName.empty(), ("Empty country name?"));
    json_object_set_new(jCountry.get(), "n", json_string(strName.c_str()));
    string const strFlag = v[i].Value().Flag();
    if (!strFlag.empty())
      json_object_set_new(jCountry.get(), "c", json_string(strFlag.c_str()));

    size_t countriesCount = v[i].Value().GetFilesCount();
    ASSERT_LESS_OR_EQUAL(countriesCount, 1, ());
    if (countriesCount > 0)
    {
      CountryFile const & file = v[i].Value().GetFile();
      string const & strFile = file.GetNameWithoutExt();
      if (strFile != strName)
        json_object_set_new(jCountry.get(), "f", json_string(strFile.c_str()));
      json_object_set_new(jCountry.get(), "s", json_integer(file.GetRemoteSize(MapOptions::Map)));
      json_object_set_new(jCountry.get(), "rs",
                          json_integer(file.GetRemoteSize(MapOptions::CarRouting)));
    }

    if (v[i].SiblingsCount())
      SaveImpl(v[i], jCountry.get());

    json_array_append(jArray.get(), jCountry.get());
  }

  json_object_set(jParent, "g", jArray.get());
}

bool SaveCountries(int64_t version, CountriesContainerT const & countries, string & jsonBuffer)
{
  my::JsonHandle root;
  root.AttachNew(json_object());

  json_object_set_new(root.get(), "v", json_integer(version));
  json_object_set_new(root.get(), "n", json_string("World"));
  SaveImpl(countries, root.get());

  char * res = json_dumps(root.get(), JSON_PRESERVE_ORDER | JSON_COMPACT | JSON_INDENT(1));
  jsonBuffer = res;
  free(res);
  return true;
}

}  // namespace storage

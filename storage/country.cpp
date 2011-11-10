#include "country.hpp"

#include "../indexer/data_factory.hpp"

#include "../coding/file_container.hpp"

#include "../platform/platform.hpp"

#include "../version/version.hpp"

#include "../base/logging.hpp"

#include "../3party/jansson/myjansson.hpp"


namespace storage
{

/// Simple check - if file is present on disk. Incomplete download has different file name.
bool IsFileDownloaded(CountryFile const & file)
{
  uint64_t size = 0;
  if (!GetPlatform().GetFileSize(GetPlatform().WritablePathForFile(file.GetFileWithExt()), size))
    return false;

  return true;//tile.second == size;
}

struct CountryBoundsCalculator
{
  m2::RectD & m_bounds;
  CountryBoundsCalculator(m2::RectD & bounds) : m_bounds(bounds) {}
  void operator()(CountryFile const & file)
  {
    feature::DataHeader h;
    LoadMapHeader(GetPlatform().GetReader(file.GetFileWithExt()), h);
    m_bounds.Add(h.GetBounds());
  }
};

m2::RectD Country::Bounds() const
{
  m2::RectD bounds;
  std::for_each(m_files.begin(), m_files.end(), CountryBoundsCalculator(bounds));
  return bounds;
}

LocalAndRemoteSizeT Country::Size() const
{
  uint64_t localSize = 0, remoteSize = 0;
  for (FilesContainerT::const_iterator it = m_files.begin(); it != m_files.end(); ++it)
  {
    if (IsFileDownloaded(*it))
      localSize += it->m_remoteSize;
    remoteSize += it->m_remoteSize;
  }
  return LocalAndRemoteSizeT(localSize, remoteSize);
}

void Country::AddFile(CountryFile const & file)
{
  m_files.push_back(file);
}

int64_t Country::Price() const
{
  int64_t price = 0;
  for (FilesContainerT::const_iterator it = m_files.begin(); it != m_files.end(); ++it)
    price += it->m_price;
  return price;
}

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
    // other fields are optional
    char const * flag = json_string_value(json_object_get(j, "c"));
    char const * file = json_string_value(json_object_get(j, "f"));
    // if file is empty, it's the same as the name
    if (!file)
      file = name;
    // price is valid only if size is not 0
    json_int_t size = json_integer_value(json_object_get(j, "s"));
    json_t * jPrice = json_object_get(j, "p");
    json_int_t price = jPrice ? json_integer_value(jPrice) : INVALID_PRICE;

    toDo(name, file, flag ? flag : "", size, price, depth);

    json_t * children = json_object_get(j, "g");
    if (children)
      LoadGroupImpl(depth + 1, children, toDo);
  }
}

template <class ToDo>
int64_t LoadCountriesImpl(string const & jsonBuffer, ToDo & toDo)
{
  int64_t version = -1;

  try
  {
    my::Json root(jsonBuffer.c_str());
    version = json_integer_value(json_object_get(root, "v"));
    json_t * children = json_object_get(root, "g");
    if (!children)
      MYTHROW(my::Json::Exception, ("Root country doesn't have any groups"));
    LoadGroupImpl(0, children, toDo);
  }
  catch (my::Json::Exception const & e)
  {
    LOG(LERROR, (e.what()));
    return -1;
  }

  return version;
}

namespace
{
  class DoStoreCountries
  {
    CountriesContainerT & m_cont;
  public:
    DoStoreCountries(CountriesContainerT & cont) : m_cont(cont) {}

    void operator() (string const & name, string const & file, string const & flag,
                     uint32_t size, int64_t price, int depth)
    {
      Country country(name, flag);
      if (size)
        country.AddFile(CountryFile(file, size, price));
      m_cont.AddAtDepth(depth, country);
    }
  };

  class DoStoreNames
  {
    map<string, string> & m_id2name;
  public:
    DoStoreNames(map<string, string> & id2name) : m_id2name(id2name) {}

    void operator() (string name, string const & file, string const &,
                     uint32_t size, int64_t, int)
    {
      // if 'file' is empty - it's equal to name
      if (size && !file.empty())
      {
        size_t const i = file.find_first_of('_');
        if (i != string::npos)
          name = file.substr(0, i) + '_' + name;

        if (name != file)
          m_id2name[file] = name;
      }
    }
  };
}

int64_t LoadCountries(string const & jsonBuffer, CountriesContainerT & countries)
{
  countries.Clear();
  DoStoreCountries doStore(countries);
  return LoadCountriesImpl(jsonBuffer, doStore);
}

void LoadCountryNames(string const & jsonBuffer, map<string, string> & id2name)
{
  ASSERT ( id2name.empty(), () );
  DoStoreNames doStore(id2name);
  LoadCountriesImpl(jsonBuffer, doStore);
}

template <class T>
void SaveImpl(T const & v, json_t * jParent)
{
  size_t const siblingsCount = v.SiblingsCount();
  CHECK_GREATER(siblingsCount, 0, ());
  my::Json jArray(json_array());
  for (size_t i = 0; i < siblingsCount; ++i)
  {
    my::Json jCountry(json_object());
    string const strName = v[i].Value().Name();
    CHECK(!strName.empty(), ("Empty country name?"));
    json_object_set_new(jCountry, "n", json_string(strName.c_str()));
    string const strFlag = v[i].Value().Flag();
    if (!strFlag.empty())
      json_object_set_new(jCountry, "c", json_string(strFlag.c_str()));
    CHECK_LESS_OR_EQUAL(v[i].Value().Files().size(), 1, ("Not supporting more than 1 file for the country at the moment"));
    if (v[i].Value().Files().size())
    {
      int64_t const price = v[i].Value().Files()[0].m_price;
      CHECK_GREATER_OR_EQUAL(price, 0, ("Invalid price"));
      json_object_set_new(jCountry, "p", json_integer(price));
      string const strFile = v[i].Value().Files()[0].m_fileName;
      if (strFile != strName)
        json_object_set_new(jCountry, "f", json_string(strFile.c_str()));
      json_object_set_new(jCountry, "s", json_integer(v[i].Value().Files()[0].m_remoteSize));
    }
    if (v[i].SiblingsCount())
      SaveImpl(v[i], jCountry);

    json_array_append(jArray, jCountry);
  }
  json_object_set(jParent, "g", jArray);
}

bool SaveCountries(int64_t version, CountriesContainerT const & countries, string & jsonBuffer)
{
  jsonBuffer.clear();
  my::Json root(json_object());
  json_object_set_new(root, "v", json_integer(version));
  json_object_set_new(root, "n", json_string("World"));
  SaveImpl(countries, root);
  char * res = json_dumps(root, JSON_PRESERVE_ORDER | JSON_COMPACT | JSON_INDENT(1));
  jsonBuffer = res;
  free(res);
  return true;
}

} // namespace storage

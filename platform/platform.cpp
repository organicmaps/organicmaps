#include "platform.hpp"

#include "../coding/sha2.hpp"
#include "../coding/base64.hpp"

#include "../base/logging.hpp"


string Platform::ReadPathForFile(string const & file, string searchScope) const
{
  if (searchScope.empty())
    searchScope = "wrfo";

  string fullPath;
  for (size_t i = 0; i < searchScope.size(); ++i)
  {
    switch (searchScope[i])
    {
    case 'w': fullPath = m_writableDir + file; break;
    case 'r': fullPath = m_resourcesDir + file; break;
    case 's': fullPath = m_settingsDir + file; break;
    case 'o': fullPath = m_optionalDir + file; break;
    case 'f': fullPath = file; break;
    default : CHECK(false, ("Unsupported searchScope:", searchScope)); break;
    }
    if (IsFileExistsByFullPath(fullPath))
      return fullPath;
  }

  MYTHROW(FileAbsentException, ("File", file, "doesn't exist in the scope", searchScope));
}

string Platform::HashUniqueID(string const & s)
{
  // generate sha2 hash for UUID
  string const hash = sha2::digest256(s, false);
  // xor it
  size_t const offset = hash.size() / 4;
  string xoredHash;
  for (size_t i = 0; i < offset; ++i)
    xoredHash.push_back(hash[i] ^ hash[i + offset] ^ hash[i + offset * 2] ^ hash[i + offset * 3]);
  // and use base64 encoding
  return base64_for_user_ids::encode(xoredHash);
}

string Platform::ResourcesMetaServerUrl() const
{
  return "http://active.resources.servers.url";
}

string Platform::MetaServerUrl() const
{
  if (IsPro())
    return "http://active.servers.url";
  else
    return "http://active.servers.url";
}

string Platform::DefaultUrlsJSON() const
{
  if (IsPro())
    return "[\"http://v2s-1.mapswithme.com/\",\"http://v2s-2.mapswithme.com/\",\"http://v2s-3.mapswithme.com/\"]";
  else
    return "[\"http://v2-1.mapswithme.com/\",\"http://v2-2.mapswithme.com/\",\"http://v2-3.mapswithme.com/\"]";
}

void Platform::GetFontNames(FilesList & res) const
{
  ASSERT(res.empty(), ());

  /// @todo Actually, this list should present once in all our code.
  /// We can take it from data/external_resources.txt
  char const * arrDef[] = {
    "00_roboto_regular.ttf",
    "01_dejavusans.ttf",
    "02_wqy-microhei.ttf",
    "03_jomolhari-id-a3d.ttf",
    "04_padauk.ttf",
    "05_khmeros.ttf",
    "06_code2000.ttf",
  };
  res.insert(res.end(), arrDef, arrDef + ARRAY_SIZE(arrDef));

  GetSystemFontNames(res);

  LOG(LINFO, ("Available font files:", (res)));
}

void Platform::GetFilesByExt(string const & directory, string const & ext, FilesList & outFiles)
{
  // Transform extension mask to regexp (.mwm -> \.mwm$)
  ASSERT ( !ext.empty(), () );
  ASSERT_EQUAL ( ext[0], '.' , () );

  GetFilesByRegExp(directory, '\\' + ext + '$', outFiles);
}

string Platform::DeviceName() const
{
  return OMIM_OS_NAME;
}

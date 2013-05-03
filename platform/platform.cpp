#include "platform.hpp"

#include "../coding/sha2.hpp"
#include "../coding/base64.hpp"

#include "../base/logging.hpp"


string Platform::ReadPathForFile(string const & file) const
{
  string fullPath = m_writableDir + file;
  if (!IsFileExistsByFullPath(fullPath))
  {
    fullPath = m_resourcesDir + file;
    if (!IsFileExistsByFullPath(fullPath))
    {
      fullPath = file;
      if (!IsFileExistsByFullPath(fullPath))
        MYTHROW(FileAbsentException, ("File doesn't exist", fullPath));
    }
  }
  return fullPath;
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
  if (m_isPro)
    return "http://active.servers.url";
  else
    return "http://active.servers.url";
}

string Platform::DefaultUrlsJSON() const
{
  if (m_isPro)
    return "[\"http://1st.default.server/\",\"http://2nd.default.server/\",\"http://3rd.default.server/\"]";
  else
    return "[\"http://1st.default.server/\",\"http://2nd.default.server/\",\"http://3rd.default.server/\"]";
}

void Platform::GetFontNames(FilesList & res) const
{
  GetSystemFontNames(res);

  string const resourcesPaths[] = { WritableDir(), ResourcesDir() };

  FilesList resourcesFonts;

  for (size_t i = 0; i < ARRAY_SIZE(resourcesPaths); ++i)
  {
    LOG(LDEBUG, ("Searching for fonts in", resourcesPaths[i]));
    GetFilesByExt(resourcesPaths[i], ".ttf", resourcesFonts);
  };

  sort(resourcesFonts.begin(), resourcesFonts.end());
  resourcesFonts.erase(unique(resourcesFonts.begin(), resourcesFonts.end()), resourcesFonts.end());
  res.insert(res.end(), resourcesFonts.begin(), resourcesFonts.end());

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

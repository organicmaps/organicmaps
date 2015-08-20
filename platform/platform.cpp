#include "platform/platform.hpp"
#include "platform/local_country_file.hpp"

#include "coding/base64.hpp"
#include "coding/file_name_utils.hpp"
#include "coding/sha2.hpp"
#include "coding/writer.hpp"

#include "base/logging.hpp"

#include "std/target_os.hpp"

#include <errno.h>

// These includes are needed only for one-time fonts statistics collection on Android (see GetFontNames).
// @TODO(AlexZ): Remove in the next release.
#ifdef OMIM_OS_ANDROID
#include "std/thread.hpp"
#include "base/scope_guard.hpp"
#include "coding/compressed_bit_vector.hpp"
#include "platform/settings.hpp"
#include "3party/Alohalytics/src/alohalytics.h"
#include "3party/freetype/include/ft2build.h"
#include FT_FREETYPE_H
#endif


// static
Platform::EError Platform::ErrnoToError()
{
  switch (errno)
  {
    case ENOENT:
      return ERR_FILE_DOES_NOT_EXIST;
    case EACCES:
      return ERR_ACCESS_FAILED;
    case ENOTEMPTY:
      return ERR_DIRECTORY_NOT_EMPTY;
    case EEXIST:
      return ERR_FILE_ALREADY_EXISTS;
    default:
      return ERR_UNKNOWN;
  }
}

string Platform::ReadPathForFile(string const & file, string searchScope) const
{
  if (searchScope.empty())
    searchScope = "wrf";

  string fullPath;
  for (size_t i = 0; i < searchScope.size(); ++i)
  {
    switch (searchScope[i])
    {
    case 'w': fullPath = m_writableDir + file; break;
    case 'r': fullPath = m_resourcesDir + file; break;
    case 's': fullPath = m_settingsDir + file; break;
    case 'f': fullPath = file; break;
    default : CHECK(false, ("Unsupported searchScope:", searchScope)); break;
    }
    if (IsFileExistsByFullPath(fullPath))
      return fullPath;
  }

  string const possiblePaths = m_writableDir  + "\n" + m_resourcesDir + "\n" + m_settingsDir;
  MYTHROW(FileAbsentException, ("File", file, "doesn't exist in the scope", searchScope,
                                "Have been looking in:\n", possiblePaths));
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
  return "http://active.servers.url";
}

string Platform::DefaultUrlsJSON() const
{
  return "[\"http://v2s-1.mapswithme.com/\",\"http://v2s-2.mapswithme.com/\",\"http://v2s-3.mapswithme.com/\"]";
}

void Platform::GetFontNames(FilesList & res) const
{
  ASSERT(res.empty(), ());

  /// @todo Actually, this list should present once in all our code.
  /// We can take it from data/external_resources.txt
  char const * arrDef[] = {
#ifndef OMIM_OS_ANDROID
    "00_roboto_regular.ttf",
#endif
    "01_dejavusans.ttf",
    "02_droidsans-fallback.ttf",
    "03_jomolhari-id-a3d.ttf",
    "04_padauk.ttf",
    "05_khmeros.ttf",
    "06_code2000.ttf",
  };
  res.insert(res.end(), arrDef, arrDef + ARRAY_SIZE(arrDef));

  GetSystemFontNames(res);

  LOG(LINFO, ("Available font files:", (res)));

#ifdef OMIM_OS_ANDROID
  // This is one-time fonts statistics collection.
  // @TODO(AlexZ): Remove it in the next release.
  constexpr char const * kFontsStatisticsKey = "AndroidFontsInfo";
  constexpr char const * kLogEventName = "AndroidFontsStats";
  bool alreadyCollected = false;
  if (!Settings::Get(kFontsStatisticsKey, alreadyCollected))
  {
    Settings::Set(kFontsStatisticsKey, true);
    // Collect all data on a separate thread.
    thread([]()
    {
      LOG(LDEBUG, ("Started fonts scan"));
      FilesList files;
      char const * kSystemFontsFolder = "/system/fonts/";
      GetFilesByRegExp(kSystemFontsFolder, "\\.[ttf|otf|ttc]", files);
      if (files.empty())
      {
        alohalytics::LogEvent(kLogEventName, {{"error", "No ttf/otc/ttc files in /system/fonts/ folder."}});
        return;
      }

      LOG(LDEBUG, ("Fonts scan init FT Library"));
      FT_Library library;
      FT_Error error = FT_Init_FreeType(&library);
      if (error)
      {
        alohalytics::LogEvent(kLogEventName, "Error initializing freetype " + std::to_string(error));
        return;
      }
      MY_SCOPE_GUARD(lib_guard, [&library] () { FT_Done_FreeType(library); });

      alohalytics::TStringMap fontsInfo;
      for (auto const & file : files)
      {
        FT_Face face;
        error = FT_New_Face(library, (kSystemFontsFolder + file).c_str(), 0, &face);
        if (error)
        {
          fontsInfo[file] = "Error " + std::to_string(error);
          return;
        }
        MY_SCOPE_GUARD(face_guard, [&face] () { FT_Done_Face(face); });
        vector<uint32_t> charcodes;
        {
          charcodes.reserve(face->num_glyphs);
          FT_UInt glyph_index;
          FT_ULong charcode = FT_Get_First_Char(face, &glyph_index);
          while (glyph_index != 0)
          {
            charcodes.push_back(static_cast<uint32_t>(charcode));
            charcode = FT_Get_Next_Char(face, charcode, &glyph_index);
          }
          sort(charcodes.begin(), charcodes.end());
          charcodes.erase(unique(charcodes.begin(), charcodes.end()), charcodes.end());
        }
        string serialBitVector;
        MemWriter<string> writer(serialBitVector);
        BuildCompressedBitVector(writer, charcodes);
        fontsInfo[file] = serialBitVector;
      }
      // Send once all collected data.
      alohalytics::LogEvent(kLogEventName, fontsInfo);
    }).detach();
  }
#endif
}

void Platform::GetFilesByExt(string const & directory, string const & ext, FilesList & outFiles)
{
  // Transform extension mask to regexp (.mwm -> \.mwm$)
  ASSERT ( !ext.empty(), () );
  ASSERT_EQUAL ( ext[0], '.' , () );

  GetFilesByRegExp(directory, '\\' + ext + '$', outFiles);
}

// static
void Platform::GetFilesByType(string const & directory, unsigned typeMask, FilesList & outFiles)
{
  FilesList allFiles;
  GetFilesByRegExp(directory, ".*", allFiles);
  for (string const & file : allFiles)
  {
    EFileType type;
    if (GetFileType(my::JoinFoldersToPath(directory, file), type) != ERR_OK)
      continue;
    if (typeMask & type)
      outFiles.push_back(file);
  }
}

string Platform::DeviceName() const
{
  return OMIM_OS_NAME;
}

void Platform::SetWritableDirForTests(string const & path)
{
  m_writableDir = my::AddSlashIfNeeded(path);
}

void Platform::SetResourceDir(string const & path)
{
  m_resourcesDir = my::AddSlashIfNeeded(path);
}

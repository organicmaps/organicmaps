#pragma once

#include "defines.hpp"

#include "coding/file_name_utils.hpp"

#include "std/string.hpp"
#include "std/vector.hpp"

namespace feature
{

struct GenerateInfo
{
  GenerateInfo()
    : m_createWorld(false), m_splitByPolygons(false),
      m_makeCoasts(false), m_emitCoasts(false), m_genAddresses(false)
  {
  }

  string GetTmpFileName(string const & fileName, char const * ext = DATA_FILE_EXTENSION_TMP) const
  {
    return my::JoinFoldersToPath(m_tmpDir, fileName + ext);
  }
  string GetTargetFileName(string const & fileName, char const * ext = DATA_FILE_EXTENSION) const
  {
    return my::JoinFoldersToPath(m_targetDir, fileName + ext);
  }
  string GetIntermediateFileName(string const & fileName, char const * ext = DATA_FILE_EXTENSION) const
  {
    return my::JoinFoldersToPath(m_intermediateDir, fileName + ext);
  }
  string GetAddressesFileName() const
  {
    return ((m_genAddresses && !m_fileName.empty()) ? GetTargetFileName(m_fileName, ADDR_FILE_EXTENSION) : string());
  }

  // Directory for .mwm.tmp files.
  string m_tmpDir;
  // Directory for result .mwm files.
  string m_targetDir;
  // Directory for all intermediate files.
  string m_intermediateDir;

  // Current generated file name if --output option is defined.
  string m_fileName;

  vector<string> m_bucketNames;

  bool m_createWorld;
  bool m_splitByPolygons;
  bool m_makeCoasts, m_emitCoasts;
  bool m_genAddresses;
};

} // namespace feature

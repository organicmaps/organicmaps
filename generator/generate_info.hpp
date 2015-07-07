#pragma once

#include "std/string.hpp"
#include "std/vector.hpp"

namespace feature
{

struct GenerateInfo
{
  GenerateInfo()
    : m_createWorld(false), m_splitByPolygons(false),
      m_makeCoasts(false), m_emitCoasts(false)
  {
  }

  // directory for .mwm.tmp files
  string m_tmpDir;
  // directory for result .mwm files
  string m_targetDir;
  // directory for all intermediate files
  string m_intermediateDir;

  string m_datFileSuffix;
  string m_addressFile;

  vector<string> m_bucketNames;

  bool m_createWorld;
  bool m_splitByPolygons;
  bool m_makeCoasts, m_emitCoasts;
};

} // namespace feature

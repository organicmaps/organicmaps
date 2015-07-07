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

  string m_tmpDir;
  string m_targetDir;
  string m_datFileSuffix;
  string m_addressFile;

  vector<string> m_bucketNames;

  bool m_createWorld;
  bool m_splitByPolygons;
  bool m_makeCoasts, m_emitCoasts;
};

} // namespace feature

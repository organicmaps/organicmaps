#pragma once

#include "../std/string.hpp"
#include "../std/vector.hpp"

namespace feature
{

struct GenerateInfo
{
  GenerateInfo() : m_createWorld(false), m_splitByPolygons(false) {}
  string m_tmpDir;
  string m_datFilePrefix;
  string m_datFileSuffix;
  vector<string> m_bucketNames;
  bool m_createWorld;
  bool m_splitByPolygons;
};

} // namespace feature

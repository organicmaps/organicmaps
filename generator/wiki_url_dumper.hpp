#pragma once

#include "base/geo_object_id.hpp"

#include <iosfwd>
#include <map>
#include <string>
#include <vector>

namespace generator
{
class WikiUrlDumper
{
public:
  explicit WikiUrlDumper(std::string const & path, std::vector<std::string> const & dataFiles);

  static void DumpOne(std::string const & path, std::ostream & stream);

  void Dump(size_t cpuCount) const;

private:
  std::string m_path;
  std::vector<std::string> m_dataFiles;
};

class WikiDataFilter
{
public:
  explicit WikiDataFilter(std::string const & path, std::vector<std::string> const & dataFiles);

  static void FilterOne(std::string const & path, std::map<base::GeoObjectId, std::string> const & idToWikiData,
                        std::ostream & stream);

  void Filter(size_t cpuCount);

private:
  std::string m_path;
  std::map<base::GeoObjectId, std::string> m_idToWikiData;
  std::vector<std::string> m_dataFiles;
};
}  // namespace generator

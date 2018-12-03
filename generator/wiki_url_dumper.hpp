#pragma once

#include <iosfwd>
#include <string>
#include <vector>

namespace generator
{
class WikiUrlDumper
{
public:
  WikiUrlDumper(std::string const & path, std::vector<std::string> const & datFiles);

  static void DumpOne(std::string const & path, std::ostream & stream);

  void Dump() const;

private:
  std::string m_path;
  std::vector<std::string> m_dataFiles;
};
}  // namespace generator

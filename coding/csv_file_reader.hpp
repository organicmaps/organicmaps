#pragma once

#include <functional>
#include <string>
#include <vector>

namespace coding
{
class CSVReader
{
public:
  struct Params
  {
    Params(){};
    bool m_shouldReadHeader = false;
    char m_delimiter = ',';
  };

  CSVReader() = default;

  using LineByLineCallback = std::function<void(std::vector<std::string> const & line)>;
  using FullFileCallback = std::function<void(std::vector<std::vector<std::string>> const & file)>;

  void ReadLineByLine(std::string const & filePath, LineByLineCallback const & fn,
                      Params const & params = {}) const;

  void ReadFullFile(std::string const & filePath, FullFileCallback const & fn,
                    Params const & params = {}) const;
};
}  // namespace coding

#pragma once

#include "coding/reader.hpp"

#include <functional>
#include <sstream>
#include <string>
#include <vector>

namespace coding
{
class CSVReader
{
public:
  struct Params
  {
    Params() {}
    bool m_readHeader = false;
    char m_delimiter = ',';
  };

  CSVReader() = default;

  using Line = std::vector<std::string>;
  using File = std::vector<Line>;
  using LineByLineCallback = std::function<void(Line && line)>;
  using FullFileCallback = std::function<void(File && file)>;

  void Read(std::istringstream & stream, LineByLineCallback const & fn,
            Params const & params = {}) const;

  void Read(std::istringstream & stream, FullFileCallback const & fn,
            Params const & params = {}) const;

  template <typename Callback>
  void Read(Reader const & reader, Callback const & fn, Params const & params = {}) const
  {
    std::string str;
    str.resize(reader.Size());
    reader.Read(0, &str[0], reader.Size());
    std::istringstream stream(str);
    Read(stream, fn, params);
  }
};
}  // namespace coding

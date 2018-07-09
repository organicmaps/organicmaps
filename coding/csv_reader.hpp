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

  using Row = std::vector<std::string>;
  using File = std::vector<Row>;
  using RowByRowCallback = std::function<void(Row && row)>;
  using FullFileCallback = std::function<void(File && file)>;

  void Read(std::istringstream & stream, RowByRowCallback const & fn,
            Params const & params = {}) const;

  void Read(std::istringstream & stream, FullFileCallback const & fn,
            Params const & params = {}) const;

  template <typename Callback>
  void Read(Reader const & reader, Callback const & fn, Params const & params = {}) const
  {
    std::string str(static_cast<size_t>(reader.Size()), '\0');
    reader.Read(0, &str[0], str.size());
    std::istringstream stream(str);
    Read(stream, fn, params);
  }
};
}  // namespace coding

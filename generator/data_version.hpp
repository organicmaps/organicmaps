#pragma once

#include "3party/jansson/myjansson.hpp"

#include <chrono>
#include <string>

#include "defines.hpp"

namespace generator
{
class DataVersion
{
public:
  static constexpr const char * kFileTag = INDEX_GENERATOR_DATA_VERSION_FILE_TAG;

  explicit DataVersion(std::string const & planetFilePath);
  std::string GetVersionJson() const;
  void DumpToPath(std::string const & path) const;
  static DataVersion LoadFromPath(std::string const & path);

private:
  DataVersion() = default;
  static std::string ReadWholeFile(std::string const & filePath);
  static std::string const & FileName();
  static std::string const & Key();
  base::JSONPtr m_json;
};
}  // namespace generator

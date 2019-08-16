#include "data_version.hpp"

#include "base/file_name_utils.hpp"
#include "base/logging.hpp"

#include "build_version.hpp"

#include <base/timer.hpp>

#include <fstream>
#include <streambuf>
#include <string>

#include <sys/stat.h>

namespace generator
{
DataVersion::DataVersion(std::string const & planetFilePath)
{
  size_t planetTimestamp = 0;

  struct stat path_stat{};
  if (::stat(planetFilePath.c_str(), &path_stat) != 0)
    planetTimestamp = base::TimeTToSecondsSinceEpoch(path_stat.st_mtime);

  m_json.reset(json_object());

  ToJSONObject(*m_json, "time_started",
               base::TimeTToSecondsSinceEpoch(
                   std::chrono::system_clock::to_time_t(std::chrono::system_clock::now())));
  ToJSONObject(*m_json, "generator_build_time", omim::build_version::git::kTimestamp);
  ToJSONObject(*m_json, "generator_git_hash", omim::build_version::git::kHash);
  ToJSONObject(*m_json, "planet_md5", ReadWholeFile(planetFilePath + ".md5"));
  ToJSONObject(*m_json, "planet_timestamp", planetTimestamp);
}

std::string DataVersion::GetVersionJson() const { return base::DumpToString(m_json); }

void DataVersion::DumpToPath(std::string const & path) const
{
  std::string filePath = base::JoinPath(path, DataVersion::FileName());
  std::ofstream stream;
  stream.exceptions(std::ios::failbit | std::ios::badbit);
  stream.open(filePath);
  stream << Key() << " " << GetVersionJson();

  LOG(LINFO, ("Version of data has been written in", filePath));
}

// static
DataVersion DataVersion::LoadFromPath(std::string const & path)
{
  DataVersion result;
  std::string filePath = base::JoinPath(path, DataVersion::FileName());
  std::string str = ReadWholeFile(filePath);

  CHECK_EQUAL(str.find(Key()), 0, ());

  result.m_json = base::LoadFromString(str.substr(Key().length()));
  LOG(LINFO, ("Version of data has been loaded from", filePath));

  return result;
}
// static
std::string const & DataVersion::FileName()
{
  static std::string const fileName = "version.jsonl";
  return fileName;
}
// static
std::string const & DataVersion::Key()
{
  static std::string const key = "version";
  return key;
}
// static
std::string DataVersion::ReadWholeFile(std::string const & filePath)
{
  std::ifstream stream;
  stream.exceptions(std::fstream::failbit | std::fstream::badbit);
  stream.open(filePath);
  return std::string((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
}
}  // namespace generator

#include "map/bookmarks_sync/base_state.hpp"

#include "platform/platform.hpp"

#include "coding/file_reader.hpp"
#include "coding/file_writer.hpp"
#include "coding/internal/file_data.hpp"

#include "base/file_name_utils.hpp"
#include "base/logging.hpp"

#include "glaze/json.hpp"

namespace bookmarks_sync
{
// Glaze reflects only types with linkage.
namespace state_json
{
struct FileJson
{
  std::string cloudId;
  std::optional<std::string> hash;
  std::optional<std::string> version;
  std::optional<std::string> md5;
};

struct StateJson
{
  int version = 0;
  std::optional<std::string> algorithm;
  std::optional<std::string> generation;
  std::optional<int64_t> lastSync;
  std::optional<int64_t> lastPurge;
  std::map<std::string, FileJson> files;
  std::map<std::string, std::string> pendingUploads;
};
}  // namespace state_json

namespace
{
using state_json::FileJson;
using state_json::StateJson;

// Version 1 was written by the Kotlin engine: MD5 only, "md5" for both the local and the cloud content.
int constexpr kStateVersion = 2;

std::string_view constexpr kMd5 = "md5";
std::string_view constexpr kBlake3 = "blake3";

std::optional<HashAlgorithm> ParseAlgorithm(std::optional<std::string> const & name)
{
  if (name == kMd5)
    return HashAlgorithm::Md5;
  if (name == kBlake3)
    return HashAlgorithm::Blake3;
  return {};
}

std::string_view ToString(HashAlgorithm algorithm)
{
  return algorithm == HashAlgorithm::Md5 ? kMd5 : kBlake3;
}
}  // namespace

BaseState LoadBaseState(std::string const & path, HashAlgorithm algorithm)
{
  BaseState state{.m_algorithm = algorithm};
  if (!Platform::IsFileExistsByFullPath(path))
    return state;

  std::string buffer;
  FileReader(path).ReadAsString(buffer);
  StateJson json;
  if (auto const error = glz::read<glz::opts{.error_on_unknown_keys = false}>(json, buffer); error)
    MYTHROW(RootException, ("Can't parse", path, glz::format_error(error, buffer)));

  std::optional<HashAlgorithm> recorded;
  if (json.version == 1)
    recorded = HashAlgorithm::Md5;
  else if (json.version == kStateVersion)
    recorded = ParseAlgorithm(json.algorithm);
  if (recorded != algorithm)
  {
    LOG(LINFO, ("Ignoring the sync state of version", json.version, "for", json.algorithm, "in", path));
    return state;
  }

  state.m_generation = std::move(json.generation);
  state.m_lastSyncMs = json.lastSync;
  state.m_lastPurgeMs = json.lastPurge;
  state.m_pendingUploads = std::move(json.pendingUploads);
  for (auto & [name, file] : json.files)
  {
    auto hash = file.hash ? std::move(*file.hash) : file.md5.value_or("");
    auto version = file.version ? std::move(*file.version) : hash;
    state.m_files.emplace(name, BaseEntry{std::move(file.cloudId), std::move(hash), std::move(version)});
  }
  return state;
}

void SaveBaseState(std::string const & path, BaseState const & state)
{
  StateJson json{.version = kStateVersion,
                 .algorithm = std::string{ToString(state.m_algorithm)},
                 .generation = state.m_generation,
                 .lastSync = state.m_lastSyncMs,
                 .lastPurge = state.m_lastPurgeMs,
                 .pendingUploads = state.m_pendingUploads};
  for (auto const & [name, entry] : state.m_files)
    json.files.emplace(name, FileJson{.cloudId = entry.m_cloudId, .hash = entry.m_hash, .version = entry.m_version});

  std::string buffer;
  if (auto const error = glz::write_json(json, buffer); error)
    MYTHROW(RootException, ("Can't serialize the sync state", glz::format_error(error)));

  if (!Platform::MkDirChecked(base::GetDirectory(path)))
    MYTHROW(RootException, ("Can't create the directory of", path));
  bool const saved = base::WriteToTempAndRenameToFile(path, [&buffer](std::string const & tmp)
  {
    try
    {
      FileWriter(tmp).Write(buffer.data(), buffer.size());
      return true;
    }
    catch (FileWriter::Exception const & e)
    {
      LOG(LWARNING, ("Can't write", tmp, e.Msg()));
      return false;
    }
  });
  if (!saved)
    MYTHROW(RootException, ("Can't save", path));
}

void DeleteBaseState(std::string const & path)
{
  base::DeleteFileX(path);
}
}  // namespace bookmarks_sync

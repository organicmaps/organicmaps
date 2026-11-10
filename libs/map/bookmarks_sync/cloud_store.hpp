#pragma once

#include "base/exception.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace bookmarks_sync
{
enum class HashAlgorithm : uint8_t
{
  Md5,
  Blake3,
};

std::string DebugPrint(HashAlgorithm algorithm);

/// A file in the cloud. m_version changes with every content change of the file m_id. m_hash is the content hash
/// in the algorithm that CloudStore::GetHashAlgorithm() reports, if the store reports hashes.
struct CloudFile
{
  std::string m_id;
  std::string m_name;
  std::string m_version;
  std::optional<std::string> m_hash;
  int64_t m_createdTimeMs = 0;
};

std::string DebugPrint(CloudFile const & file);

enum class SyncError : uint8_t
{
  AuthRequired,
  QuotaExceeded,
  // Network problems, rate limits and server errors that a later retry may fix.
  Transient,
  Cancelled,
  Other,
};

std::string DebugPrint(SyncError error);

class CloudException : public RootException
{
public:
  CloudException(SyncError error, char const * what, std::string const & msg) : RootException(what, msg), m_error(error)
  {}

  SyncError GetError() const { return m_error; }

private:
  SyncError m_error;
};

/// Storage of category files in the cloud. Calls block and throw CloudException.
class CloudStore
{
public:
  virtual ~CloudStore() = default;

  /// Nothing for a store that doesn't report content hashes.
  virtual std::optional<HashAlgorithm> GetHashAlgorithm() const = 0;

  /// The top-level files, without folders and trashed files. The listing can lag behind.
  virtual std::vector<CloudFile> List() = 0;
  /// Nothing once the file is deleted or trashed, even while a listing still shows it.
  virtual std::optional<CloudFile> Stat(std::string const & id) = 0;

  /// Creates an empty file named kGenerationFileName. @return its id.
  virtual std::string CreateGeneration() = 0;
  /// @return false if the file is gone.
  virtual bool Download(std::string const & id, std::string const & destPath) = 0;
  /// Creates a file when existingId is empty, otherwise replaces its content.
  virtual CloudFile Upload(std::string const & name, std::string const & srcPath,
                           std::optional<std::string> const & existingId) = 0;

  /// Moves the file to a trash that the store purges with PurgeTrash().
  virtual void MoveToTrash(std::string const & id) = 0;
  virtual void PurgeTrash(int64_t trashedBeforeMs) = 0;

  /// Deletes the generation first, as a listing can lag behind and miss it, then the other generations and files,
  /// so other devices never take a partly deleted cloud for deletions.
  virtual void DeleteAll(std::optional<std::string> const & generationId) = 0;
};

std::string_view constexpr kGenerationFileName = ".generation";
}  // namespace bookmarks_sync

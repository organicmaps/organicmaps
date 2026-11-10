#pragma once

#include "map/bookmarks_sync/cloud_store.hpp"

#include <string>

namespace bookmarks_sync
{
/// Lowercase hex for MD5, as Google Drive reports it. Throws Reader::Exception if the file can't be read.
std::string HashFile(std::string const & path, HashAlgorithm algorithm);
}  // namespace bookmarks_sync

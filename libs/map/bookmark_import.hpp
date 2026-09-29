#pragma once

#include "kml/types.hpp"

#include <functional>
#include <string>
#include <utility>
#include <vector>

struct BookmarkFileLoadingContext
{
  BookmarkFileLoadingContext() = default;
  BookmarkFileLoadingContext(std::string filePath, bool isTemporaryFile = false,
                             std::string ownedTemporaryDirectory = {})
    : m_filePath(std::move(filePath))
    , m_isTemporaryFile(isTemporaryFile)
    , m_ownedTemporaryDirectory(std::move(ownedTemporaryDirectory))
  {}

  std::string m_filePath;
  bool m_isTemporaryFile = false;
  // Exact private staging directory created by the caller; empty for unowned paths.
  std::string m_ownedTemporaryDirectory;
};

struct BookmarkImportSourceResult
{
  BookmarkFileLoadingContext m_context;
  kml::GroupIdCollection m_groupIds;
  std::vector<std::string> m_failedFileNames;
};

struct BookmarkImportResult
{
  std::vector<BookmarkImportSourceResult> m_sourceResults;
};

struct BookmarkLoadingCallbacks
{
  std::function<void()> m_onStarted;
  std::function<void()> m_onFinished;
  std::function<void(BookmarkImportResult const &)> m_onImportFinished;
};

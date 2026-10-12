#pragma once

#include "kml/types.hpp"

#include <functional>
#include <string>
#include <vector>

struct BookmarkFileLoadingContext
{
  std::string m_filePath;
  bool m_isTemporaryFile = false;
  bool m_ownsParentDirectory = false;
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

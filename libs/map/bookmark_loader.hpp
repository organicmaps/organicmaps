#pragma once

#include "map/bookmark_import.hpp"

#include "base/thread_checker.hpp"

#include <atomic>
#include <functional>
#include <list>
#include <memory>
#include <string>
#include <utility>
#include <vector>

class BookmarkLoader final
{
public:
  using KMLDataCollection = std::vector<std::pair<std::string, std::unique_ptr<kml::FileData>>>;
  using KMLDataCollectionPtr = std::shared_ptr<KMLDataCollection>;
  using ApplyData = std::function<kml::GroupIdCollection(KMLDataCollection &&, bool isInitialLoad)>;

  explicit BookmarkLoader(ApplyData applyData);

  void SetCallbacks(BookmarkLoadingCallbacks && callbacks);
  bool IsLoadingInProgress() const { return m_asyncLoadingInProgress; }
  void Teardown() { m_needTeardown = true; }

  void LoadBookmarks();
  void ImportBookmarks(std::vector<BookmarkFileLoadingContext> contexts);
  void ReloadBookmarks(std::vector<std::string> filePaths);
  static KMLDataCollectionPtr LoadKmlFiles(std::string const & directory);

private:
  enum class RequestType
  {
    Import,
    Reload
  };

  struct Request
  {
    RequestType m_type;
    std::vector<BookmarkFileLoadingContext> m_contexts;
  };

  struct ImportSourceData
  {
    BookmarkFileLoadingContext m_context;
    KMLDataCollection m_kmlData;
    std::vector<std::string> m_failedFileNames;
  };
  using ImportSourceDataCollection = std::vector<ImportSourceData>;

  void EnqueueRequest(Request && request);
  void ProcessNextRequest();
  void ImportRoutine(std::vector<BookmarkFileLoadingContext> && contexts);
  void ReloadRoutine(std::vector<BookmarkFileLoadingContext> && contexts);
  void NotifyAboutStart();
  void NotifyAboutFinish(KMLDataCollectionPtr && collection);
  void FinishImport(ImportSourceDataCollection && dataCollection);
  void FinishRequest(BookmarkImportResult const * importResult = nullptr);

  ThreadChecker m_threadChecker;
  ApplyData m_applyData;
  BookmarkLoadingCallbacks m_callbacks;
  std::atomic<bool> m_needTeardown = false;
  bool m_loadBookmarksCalled = false;
  bool m_loadBookmarksFinished = false;
  bool m_asyncLoadingInProgress = false;
  bool m_finishingRequest = false;
  std::list<Request> m_queue;
};

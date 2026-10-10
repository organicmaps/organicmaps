#include "map/bookmark_loader.hpp"

#include "map/bookmark_helpers.hpp"

#include "platform/platform.hpp"

#include "base/assert.hpp"
#include "base/file_name_utils.hpp"
#include "base/logging.hpp"

BookmarkLoader::BookmarkLoader(ApplyData applyData) : m_applyData(std::move(applyData)) {}

void BookmarkLoader::SetCallbacks(BookmarkLoadingCallbacks && callbacks)
{
  m_callbacks = std::move(callbacks);
}

BookmarkLoader::KMLDataCollectionPtr BookmarkLoader::LoadKmlFiles(std::string const & directory)
{
  Platform::FilesList files;
  Platform::GetFilesByExt(directory, kKmlExtension, files);

  auto collection = std::make_shared<KMLDataCollection>();
  collection->reserve(files.size());
  for (auto const & file : files)
  {
    auto const filePath = base::JoinPath(directory, file);
    auto kmlData = LoadKmlFile(filePath, FileType::Kml);
    if (kmlData == nullptr)
      continue;
    collection->emplace_back(filePath, std::move(kmlData));
  }
  return collection;
}

void BookmarkLoader::LoadBookmarks()
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  CHECK(!m_loadBookmarksCalled, ("LoadBookmarks should be called only once."));
  m_loadBookmarksCalled = true;

  NotifyAboutStart();
  GetPlatform().RunTask(Platform::Thread::File, [this]()
  {
    auto collection = LoadKmlFiles(GetBookmarksDirectory());

    if (m_needTeardown)
      return;
    NotifyAboutFinish(std::move(collection));
  });
}

void BookmarkLoader::ImportBookmarks(std::vector<BookmarkFileLoadingContext> contexts)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  std::erase_if(contexts, [](auto const & context) { return context.m_filePath.empty(); });

  if (!contexts.empty())
    EnqueueRequest({RequestType::Import, std::move(contexts)});
}

void BookmarkLoader::ReloadBookmarks(std::vector<std::string> filePaths)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  std::vector<BookmarkFileLoadingContext> contexts;
  contexts.reserve(filePaths.size());
  for (auto & filePath : filePaths)
    if (!filePath.empty())
      contexts.push_back({std::move(filePath), false /* isTemporaryFile */});

  if (!contexts.empty())
    EnqueueRequest({RequestType::Reload, std::move(contexts)});
}

void BookmarkLoader::EnqueueRequest(Request && request)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  m_queue.push_back(std::move(request));

  if (m_finishingRequest)
  {
    m_asyncLoadingInProgress = true;
    return;
  }

  if (!m_loadBookmarksFinished || m_asyncLoadingInProgress)
    return;

  ProcessNextRequest();
}

void BookmarkLoader::ProcessNextRequest()
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  ASSERT(!m_queue.empty(), ());

  auto request = std::move(m_queue.front());
  m_queue.pop_front();

  NotifyAboutStart();

  switch (request.m_type)
  {
  case RequestType::Import: ImportRoutine(std::move(request.m_contexts)); break;
  case RequestType::Reload: ReloadRoutine(std::move(request.m_contexts)); break;
  }
}

void BookmarkLoader::ImportRoutine(std::vector<BookmarkFileLoadingContext> && contexts)
{
  GetPlatform().RunTask(Platform::Thread::File, [this, contexts = std::move(contexts)]() mutable
  {
    if (m_needTeardown)
      return;

    ImportSourceDataCollection dataCollection;
    dataCollection.reserve(contexts.size());

    for (auto & context : contexts)
    {
      auto importData = LoadBookmarkFileForImport(context.m_filePath);
      dataCollection.push_back(
          {std::move(context), std::move(importData.m_kmlData), std::move(importData.m_failedFileNames)});

      if (m_needTeardown)
        return;
    }

    auto dataCollectionPtr = std::make_shared<ImportSourceDataCollection>(std::move(dataCollection));
    GetPlatform().RunTask(Platform::Thread::Gui,
                          [this, dataCollectionPtr]() mutable { FinishImport(std::move(*dataCollectionPtr)); });
  });
}

void BookmarkLoader::ReloadRoutine(std::vector<BookmarkFileLoadingContext> && contexts)
{
  GetPlatform().RunTask(Platform::Thread::File, [this, contexts = std::move(contexts)]()
  {
    if (m_needTeardown)
      return;

    auto collection = std::make_shared<KMLDataCollection>();
    collection->reserve(contexts.size());
    for (auto const & context : contexts)
    {
      std::unique_ptr<kml::FileData> kmlData;
      if (auto const fileType = GetFileType(context.m_filePath))
      {
        switch (*fileType)
        {
        case FileType::Kml:
        case FileType::Gpx: kmlData = LoadKmlFile(context.m_filePath, *fileType); break;
        default: ASSERT(false, ("Unsupported bookmarks file type", (*fileType)));
        }
      }
      else
        ASSERT(false, ("Unknown file type for", context.m_filePath));

      if (kmlData)
        collection->emplace_back(context.m_filePath, std::move(kmlData));

      if (m_needTeardown)
        return;
    }

    NotifyAboutFinish(std::move(collection));
  });
}

void BookmarkLoader::NotifyAboutStart()
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  if (m_needTeardown)
    return;

  m_asyncLoadingInProgress = true;
  if (m_callbacks.m_onStarted != nullptr)
    m_callbacks.m_onStarted();
}

void BookmarkLoader::NotifyAboutFinish(KMLDataCollectionPtr && collection)
{
  if (m_needTeardown)
    return;

  GetPlatform().RunTask(Platform::Thread::Gui, [this, collection = std::move(collection)]() mutable
  {
    m_applyData(std::move(*collection), !m_loadBookmarksFinished);
    m_loadBookmarksFinished = true;
    FinishRequest();
  });
}

void BookmarkLoader::FinishImport(ImportSourceDataCollection && dataCollection)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  BookmarkImportResult result;
  result.m_sourceResults.reserve(dataCollection.size());

  KMLDataCollection categories;
  std::vector<size_t> sourceIndexes;
  for (size_t sourceIndex = 0; sourceIndex < dataCollection.size(); ++sourceIndex)
  {
    auto & data = dataCollection[sourceIndex];
    result.m_sourceResults.push_back({std::move(data.m_context), {}, std::move(data.m_failedFileNames)});

    for (auto & kmlData : data.m_kmlData)
    {
      categories.push_back(std::move(kmlData));
      sourceIndexes.push_back(sourceIndex);
    }
  }

  if (!categories.empty())
  {
    auto const groupIds = m_applyData(std::move(categories), false /* isInitialLoad */);
    CHECK_EQUAL(groupIds.size(), sourceIndexes.size(), ());
    for (size_t i = 0; i < groupIds.size(); ++i)
      result.m_sourceResults[sourceIndexes[i]].m_groupIds.push_back(groupIds[i]);
  }

  for (auto const & source : result.m_sourceResults)
  {
    if (!source.m_context.m_isTemporaryFile || source.m_context.m_filePath.empty())
      continue;

    auto const & path = source.m_context.m_filePath;
    if (!Platform::RemoveFileIfExists(path))
      LOG(LWARNING, ("Failed to delete temporary bookmarks file:", path));

    if (!source.m_context.m_ownsParentDirectory)
      continue;
    auto const directory = base::GetDirectory(path);
    if (Platform::RmDir(directory) != Platform::ERR_OK)
      LOG(LWARNING, ("Failed to delete temporary bookmarks directory:", directory));
  }

  FinishRequest(&result);
}

void BookmarkLoader::FinishRequest(BookmarkImportResult const * importResult)
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  m_asyncLoadingInProgress = !m_queue.empty();
  m_finishingRequest = true;

  if (m_callbacks.m_onFinished != nullptr)
    m_callbacks.m_onFinished();

  if (importResult != nullptr && m_callbacks.m_onImportFinished != nullptr)
    m_callbacks.m_onImportFinished(*importResult);

  m_finishingRequest = false;
  if (m_queue.empty())
    return;

  ProcessNextRequest();
}

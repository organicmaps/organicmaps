#include "testing/testing.hpp"

#include "platform/http_request.hpp"
#include "platform/chunks_download_strategy.hpp"
#include "platform/platform.hpp"

#include "coding/file_reader.hpp"
#include "coding/file_writer.hpp"
#include "coding/internal/file_data.hpp"

#include "base/logging.hpp"
#include "base/std_serialization.hpp"

#include <QtCore/QCoreApplication>

#include <functional>
#include <memory>
#include <vector>

#include "defines.hpp"

namespace downloader_test
{
using namespace downloader;
using namespace std::placeholders;
using namespace std;

char constexpr kTestUrl1[] = "http://localhost:34568/unit_tests/1.txt";
char constexpr kTestUrl404[] = "http://localhost:34568/unit_tests/notexisting_unittest";
char constexpr kTestUrlBigFile[] = "http://localhost:34568/unit_tests/47kb.file";

// Should match file size in tools/python/ResponseProvider.py
int constexpr kBigFileSize = 47684;

class DownloadObserver
{
  bool m_progressWasCalled;
  // Chunked downloads can return one status per chunk (thread).
  vector<DownloadStatus> m_statuses;
  // Interrupt download after this number of chunks
  int m_chunksToFail;
  base::ScopedLogLevelChanger const m_debugLogLevel;

public:
  DownloadObserver() : m_chunksToFail(-1), m_debugLogLevel(LDEBUG)
  {
    Reset();
  }

  void CancelDownloadOnGivenChunk(int chunksToFail)
  {
    m_chunksToFail = chunksToFail;
  }

  void Reset()
  {
    m_progressWasCalled = false;
    m_statuses.clear();
  }

  void TestOk()
  {
    TEST_NOT_EQUAL(0, m_statuses.size(), ("Observer was not called."));
    TEST(m_progressWasCalled, ("Download progress wasn't called"));
    for (auto const & status : m_statuses)
      TEST_EQUAL(status, DownloadStatus::Completed, ());
  }

  void TestFailed()
  {
    TEST_NOT_EQUAL(0, m_statuses.size(), ("Observer was not called."));
    for (auto const & status : m_statuses)
      TEST_EQUAL(status, DownloadStatus::Failed, ());
  }

  void TestFileNotFound()
  {
    TEST_NOT_EQUAL(0, m_statuses.size(), ("Observer was not called."));
    for (auto const & status : m_statuses)
      TEST_EQUAL(status, DownloadStatus::FileNotFound, ());
  }

  void OnDownloadProgress(HttpRequest & request)
  {
    m_progressWasCalled = true;
    TEST_EQUAL(request.GetStatus(), DownloadStatus::InProgress, ());

    // Cancel download if needed
    if (m_chunksToFail != -1)
    {
      --m_chunksToFail;
      if (m_chunksToFail == 0)
      {
        m_chunksToFail = -1;
        LOG(LINFO, ("Download canceled"));
        QCoreApplication::quit();
      }
    }
  }

  virtual void OnDownloadFinish(HttpRequest & request)
  {
    auto const status = request.GetStatus();
    m_statuses.emplace_back(status);
    TEST(status != DownloadStatus::InProgress, ());
    QCoreApplication::quit();
  }
};

struct CancelDownload
{
  void OnProgress(HttpRequest & request)
  {
    TEST_GREATER(request.GetData().size(), 0, ());
    delete &request;
    QCoreApplication::quit();
  }
  void OnFinish(HttpRequest &)
  {
    TEST(false, ("Should be never called"));
  }
};

struct DeleteOnFinish
{
  void OnProgress(HttpRequest & request)
  {
    TEST_GREATER(request.GetData().size(), 0, ());
  }
  void OnFinish(HttpRequest & request)
  {
    delete &request;
    QCoreApplication::quit();
  }
};

UNIT_TEST(DownloaderSimpleGet)
{
  DownloadObserver observer;
  auto const MakeRequest = [&observer](std::string const & url)
  {
    return HttpRequest::Get(url,
                            bind(&DownloadObserver::OnDownloadFinish, &observer, _1),
                            bind(&DownloadObserver::OnDownloadProgress, &observer, _1));
  };

  {
    // simple success case
    unique_ptr<HttpRequest> const request {MakeRequest(kTestUrl1)};
    // wait until download is finished
    QCoreApplication::exec();
    observer.TestOk();
    TEST_EQUAL(request->GetData(), "Test1", ());
  }

  observer.Reset();
  {
    // We DO NOT SUPPORT redirects to avoid data corruption when downloading mwm files
    unique_ptr<HttpRequest> const request {MakeRequest("http://localhost:34568/unit_tests/permanent")};
    QCoreApplication::exec();
    observer.TestFailed();
    TEST_EQUAL(request->GetData().size(), 0, (request->GetData()));
  }

  observer.Reset();
  {
    // fail case 404
    unique_ptr<HttpRequest> const request {MakeRequest(kTestUrl404)};
    QCoreApplication::exec();
    observer.TestFileNotFound();
    TEST_EQUAL(request->GetData().size(), 0, (request->GetData()));
  }

  observer.Reset();
  {
    // fail case not existing host
    unique_ptr<HttpRequest> const request {MakeRequest("http://not-valid-host123532.ath.cx")};
    QCoreApplication::exec();
    observer.TestFailed();
    TEST_EQUAL(request->GetData().size(), 0, (request->GetData()));
  }

  {
    // cancel download in the middle of the progress
    CancelDownload canceler;
    // should be deleted in canceler
    HttpRequest::Get(kTestUrlBigFile,
        bind(&CancelDownload::OnFinish, &canceler, _1),
        bind(&CancelDownload::OnProgress, &canceler, _1));
    QCoreApplication::exec();
  }

  observer.Reset();
  {
    // https success case
    unique_ptr<HttpRequest> const request {MakeRequest("https://github.com")};
    // wait until download is finished
    QCoreApplication::exec();
    observer.TestOk();
    TEST_GREATER(request->GetData().size(), 0, ());
  }

  {
    // Delete request at the end of successful download
    DeleteOnFinish deleter;
    // should be deleted in deleter on finish
    HttpRequest::Get(kTestUrl1,
        bind(&DeleteOnFinish::OnFinish, &deleter, _1),
        bind(&DeleteOnFinish::OnProgress, &deleter, _1));
    QCoreApplication::exec();
  }
}

UNIT_TEST(DownloaderSimplePost)
{
  // simple success case
  string const postData = "{\"jsonKey\":\"jsonValue\"}";
  DownloadObserver observer;
  unique_ptr<HttpRequest> const request {HttpRequest::PostJson("http://localhost:34568/unit_tests/post.php", postData,
        bind(&DownloadObserver::OnDownloadFinish, &observer, _1),
        bind(&DownloadObserver::OnDownloadProgress, &observer, _1))};
  // wait until download is finished
  QCoreApplication::exec();
  observer.TestOk();
  TEST_EQUAL(request->GetData(), postData, ());
}

UNIT_TEST(ChunksDownloadStrategy)
{
  vector<string> const servers = {"UrlOfServer1", "UrlOfServer2", "UrlOfServer3"};

  typedef pair<int64_t, int64_t> RangeT;
  RangeT const R1{0, 249}, R2{250, 499}, R3{500, 749}, R4{750, 800};

  int64_t constexpr kFileSize = 800;
  int64_t constexpr kChunkSize = 250;
  ChunksDownloadStrategy strategy(servers);
  strategy.InitChunks(kFileSize, kChunkSize);

  string s1;
  RangeT r1;
  TEST_EQUAL(strategy.NextChunk(s1, r1), ChunksDownloadStrategy::ENextChunk, ());

  string s2;
  RangeT r2;
  TEST_EQUAL(strategy.NextChunk(s2, r2), ChunksDownloadStrategy::ENextChunk, ());

  string s3;
  RangeT r3;
  TEST_EQUAL(strategy.NextChunk(s3, r3), ChunksDownloadStrategy::ENextChunk, ());

  string sEmpty;
  RangeT rEmpty;
  TEST_EQUAL(strategy.NextChunk(sEmpty, rEmpty), ChunksDownloadStrategy::ENoFreeServers, ());
  TEST_EQUAL(strategy.NextChunk(sEmpty, rEmpty), ChunksDownloadStrategy::ENoFreeServers, ());

  TEST(s1 != s2 && s2 != s3 && s3 != s1, (s1, s2, s3));

  TEST(r1 != r2 && r2 != r3 && r3 != r1, (r1, r2, r3));
  TEST(r1 == R1 || r1 == R2 || r1 == R3 || r1 == R4, (r1));
  TEST(r2 == R1 || r2 == R2 || r2 == R3 || r2 == R4, (r2));
  TEST(r3 == R1 || r3 == R2 || r3 == R3 || r3 == R4, (r3));

  strategy.ChunkFinished(true, r1);

  string s4;
  RangeT r4;
  TEST_EQUAL(strategy.NextChunk(s4, r4), ChunksDownloadStrategy::ENextChunk, ());
  TEST_EQUAL(s4, s1, ());
  TEST(r4 != r1 && r4 != r2 && r4 != r3, (r4));

  TEST_EQUAL(strategy.NextChunk(sEmpty, rEmpty), ChunksDownloadStrategy::ENoFreeServers, ());
  TEST_EQUAL(strategy.NextChunk(sEmpty, rEmpty), ChunksDownloadStrategy::ENoFreeServers, ());

  strategy.ChunkFinished(false, r2);

  TEST_EQUAL(strategy.NextChunk(sEmpty, rEmpty), ChunksDownloadStrategy::ENoFreeServers, ());

  strategy.ChunkFinished(true, r4);

  string s5;
  RangeT r5;
  TEST_EQUAL(strategy.NextChunk(s5, r5), ChunksDownloadStrategy::ENextChunk, ());
  TEST_EQUAL(s5, s4, (s5, s4));
  TEST_EQUAL(r5, r2, ());

  TEST_EQUAL(strategy.NextChunk(sEmpty, rEmpty), ChunksDownloadStrategy::ENoFreeServers, ());
  TEST_EQUAL(strategy.NextChunk(sEmpty, rEmpty), ChunksDownloadStrategy::ENoFreeServers, ());

  strategy.ChunkFinished(true, r5);

  // 3rd is still alive here
  TEST_EQUAL(strategy.NextChunk(sEmpty, rEmpty), ChunksDownloadStrategy::ENoFreeServers, ());
  TEST_EQUAL(strategy.NextChunk(sEmpty, rEmpty), ChunksDownloadStrategy::ENoFreeServers, ());

  strategy.ChunkFinished(true, r3);

  TEST_EQUAL(strategy.NextChunk(sEmpty, rEmpty), ChunksDownloadStrategy::EDownloadSucceeded, ());
  TEST_EQUAL(strategy.NextChunk(sEmpty, rEmpty), ChunksDownloadStrategy::EDownloadSucceeded, ());
}

UNIT_TEST(ChunksDownloadStrategyFAIL)
{
  vector<string> const servers = {"UrlOfServer1", "UrlOfServer2"};

  typedef pair<int64_t, int64_t> RangeT;

  int64_t constexpr kFileSize = 800;
  int64_t constexpr kChunkSize = 250;
  ChunksDownloadStrategy strategy(servers);
  strategy.InitChunks(kFileSize, kChunkSize);

  string s1;
  RangeT r1;
  TEST_EQUAL(strategy.NextChunk(s1, r1), ChunksDownloadStrategy::ENextChunk, ());

  string s2;
  RangeT r2;
  TEST_EQUAL(strategy.NextChunk(s2, r2), ChunksDownloadStrategy::ENextChunk, ());
  TEST_EQUAL(strategy.NextChunk(s2, r2), ChunksDownloadStrategy::ENoFreeServers, ());

  strategy.ChunkFinished(false, r1);

  TEST_EQUAL(strategy.NextChunk(s2, r2), ChunksDownloadStrategy::ENoFreeServers, ());

  strategy.ChunkFinished(false, r2);

  TEST_EQUAL(strategy.NextChunk(s2, r2), ChunksDownloadStrategy::EDownloadFailed, ());
}

namespace
{
string ReadFileAsString(string const & file)
{
  try
  {
    FileReader f(file);
    string s;
    f.ReadAsString(s);
    return s;
  }
  catch (FileReader::Exception const &)
  {
    TEST(false, ("File ", file, " should exist"));
    return string();
  }
}  // namespace

void FinishDownloadSuccess(string const & file)
{
  TEST(base::DeleteFileX(file), ("Result file should present on success"));

  uint64_t size;
  TEST(!base::GetFileSize(file + DOWNLOADING_FILE_EXTENSION, size), ("No downloading file on success"));
  TEST(!base::GetFileSize(file + RESUME_FILE_EXTENSION, size), ("No resume file on success"));
}

void FinishDownloadFail(string const & file)
{
  uint64_t size;
  TEST(!base::GetFileSize(file, size), ("No result file on fail"));

  (void)base::DeleteFileX(file + DOWNLOADING_FILE_EXTENSION);

  TEST(base::DeleteFileX(file + RESUME_FILE_EXTENSION), ("Resume file should present on fail"));
}

void DeleteTempDownloadFiles()
{
  // Remove data from previously failed files.

  // Get regexp like this: (\.downloading3$|\.resume3$)
  string const regexp = "(\\" RESUME_FILE_EXTENSION "$|\\" DOWNLOADING_FILE_EXTENSION "$)";

  Platform::FilesList files;
  Platform::GetFilesByRegExp(".", regexp, files);
  for (Platform::FilesList::iterator it = files.begin(); it != files.end(); ++it)
    FileWriter::DeleteFileX(*it);
}
}  // namespace


UNIT_TEST(DownloadChunks)
{
  string const kFileName = "some_downloader_test_file";

  // remove data from previously failed files
  DeleteTempDownloadFiles();

  vector<string> urls = {kTestUrl1, kTestUrl1};
  int64_t fileSize = 5;

  DownloadObserver observer;
  auto const MakeRequest = [&](int64_t chunkSize)
  {
    return HttpRequest::GetFile(urls, kFileName, fileSize,
                                bind(&DownloadObserver::OnDownloadFinish, &observer, _1),
                                bind(&DownloadObserver::OnDownloadProgress, &observer, _1),
                                chunkSize);
  };

  {
    // should use only one thread
    unique_ptr<HttpRequest> const request {MakeRequest(512 * 1024)};
    // wait until download is finished
    QCoreApplication::exec();
    observer.TestOk();
    TEST_EQUAL(request->GetData(), kFileName, ());
    TEST_EQUAL(ReadFileAsString(kFileName), "Test1", ());
    FinishDownloadSuccess(kFileName);
  }

  observer.Reset();
  urls = {kTestUrlBigFile, kTestUrlBigFile, kTestUrlBigFile};
  fileSize = 5;
  {
    // 3 threads - fail, because of invalid size
    [[maybe_unused]] unique_ptr<HttpRequest> const request {MakeRequest(2048)};
    // wait until download is finished
    QCoreApplication::exec();
    observer.TestFailed();
    FinishDownloadFail(kFileName);
  }

  observer.Reset();
  urls = {kTestUrlBigFile, kTestUrlBigFile, kTestUrlBigFile};
  fileSize = kBigFileSize;
  {
    // 3 threads - succeeded
    [[maybe_unused]] unique_ptr<HttpRequest> const request {MakeRequest(2048)};
    // wait until download is finished
    QCoreApplication::exec();
    observer.TestOk();
    FinishDownloadSuccess(kFileName);
  }

  observer.Reset();
  urls = {kTestUrlBigFile, kTestUrl1, kTestUrl404};
  fileSize = kBigFileSize;
  {
    // 3 threads with only one valid url - succeeded
    [[maybe_unused]] unique_ptr<HttpRequest> const request {MakeRequest(2048)};
    // wait until download is finished
    QCoreApplication::exec();
    observer.TestOk();
    FinishDownloadSuccess(kFileName);
  }

  observer.Reset();
  urls = {kTestUrlBigFile, kTestUrlBigFile};
  fileSize = 12345;
  {
    // 2 threads and all points to file with invalid size - fail
    [[maybe_unused]] unique_ptr<HttpRequest> const request {MakeRequest(2048)};
    // wait until download is finished
    QCoreApplication::exec();
    observer.TestFailed();
    FinishDownloadFail(kFileName);
  }
}


namespace
{
int64_t constexpr beg1 = 123, end1 = 1230, beg2 = 44000, end2 = 47683;

struct ResumeChecker
{
  size_t m_counter;
  ResumeChecker() : m_counter(0) {}

  void OnProgress(HttpRequest & request)
  {
    if (m_counter == 0)
    {
      TEST_EQUAL(request.GetProgress().m_bytesDownloaded, beg2, ());
      TEST_EQUAL(request.GetProgress().m_bytesTotal, kBigFileSize, ());
    }
    else if (m_counter == 1)
    {
      TEST_EQUAL(request.GetProgress().m_bytesDownloaded, kBigFileSize, ());
      TEST_EQUAL(request.GetProgress().m_bytesTotal, kBigFileSize, ());
    }
    else
    {
      TEST(false, ("Progress should be called exactly 2 times"));
    }
    ++m_counter;
  }

  void OnFinish(HttpRequest & request)
  {
    TEST_EQUAL(request.GetStatus(), DownloadStatus::Completed, ());
    QCoreApplication::exit();
  }
};
}  // namespace


UNIT_TEST(DownloadResumeChunks)
{
  string const FILENAME = "some_test_filename_12345";
  string const RESUME_FILENAME = FILENAME + RESUME_FILE_EXTENSION;
  string const DOWNLOADING_FILENAME = FILENAME + DOWNLOADING_FILE_EXTENSION;

  // remove data from previously failed files
  DeleteTempDownloadFiles();

  vector<string> urls = {kTestUrlBigFile};

  // 1st step - download full file
  {
    DownloadObserver observer;

    unique_ptr<HttpRequest> const request(HttpRequest::GetFile(urls, FILENAME, kBigFileSize,
                            bind(&DownloadObserver::OnDownloadFinish, &observer, _1),
                            bind(&DownloadObserver::OnDownloadProgress, &observer, _1)));

    QCoreApplication::exec();

    observer.TestOk();

    uint64_t size;
    TEST(!base::GetFileSize(RESUME_FILENAME, size), ("No resume file on success"));
  }

  // 2nd step - mark some file blocks as not downloaded
  {
    // to substitute temporary not fully downloaded file
    TEST(base::RenameFileX(FILENAME, DOWNLOADING_FILENAME), ());

    FileWriter f(DOWNLOADING_FILENAME, FileWriter::OP_WRITE_EXISTING);
    f.Seek(beg1);
    char b1[end1 - beg1 + 1] = {0};
    f.Write(b1, ARRAY_SIZE(b1));

    f.Seek(beg2);
    char b2[end2 - beg2 + 1] = {0};
    f.Write(b2, ARRAY_SIZE(b2));

    ChunksDownloadStrategy strategy((vector<string>()));
    strategy.AddChunk(make_pair(int64_t(0), beg1-1), ChunksDownloadStrategy::CHUNK_COMPLETE);
    strategy.AddChunk(make_pair(beg1, end1), ChunksDownloadStrategy::CHUNK_FREE);
    strategy.AddChunk(make_pair(end1+1, beg2-1), ChunksDownloadStrategy::CHUNK_COMPLETE);
    strategy.AddChunk(make_pair(beg2, end2), ChunksDownloadStrategy::CHUNK_FREE);

    strategy.SaveChunks(kBigFileSize, RESUME_FILENAME);
  }

  // 3rd step - check that resume works
  {
    ResumeChecker checker;
    unique_ptr<HttpRequest> const request(HttpRequest::GetFile(urls, FILENAME, kBigFileSize,
                                                         bind(&ResumeChecker::OnFinish, &checker, _1),
                                                         bind(&ResumeChecker::OnProgress, &checker, _1)));
    QCoreApplication::exec();

    FinishDownloadSuccess(FILENAME);
  }
}

// Unit test with forcible canceling of http request
UNIT_TEST(DownloadResumeChunksWithCancel)
{
  string const FILENAME = "some_test_filename_12345";

  // remove data from previously failed files
  DeleteTempDownloadFiles();

  vector<string> urls = {kTestUrlBigFile};

  DownloadObserver observer;

  int arrCancelChunks[] = { 1, 3, 10, 15, 20, 0 };

  for (size_t i = 0; i < ARRAY_SIZE(arrCancelChunks); ++i)
  {
    if (arrCancelChunks[i] > 0)
      observer.CancelDownloadOnGivenChunk(arrCancelChunks[i]);

    unique_ptr<HttpRequest> const request(HttpRequest::GetFile(urls, FILENAME, kBigFileSize,
                            bind(&DownloadObserver::OnDownloadFinish, &observer, _1),
                            bind(&DownloadObserver::OnDownloadProgress, &observer, _1),
                            1024, false));

    QCoreApplication::exec();
  }

  observer.TestOk();

  FinishDownloadSuccess(FILENAME);
}

}  // namespace downloader_test
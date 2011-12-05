#include "../../testing/testing.hpp"

#include "../../base/logging.hpp"
#include "../../base/std_serialization.hpp"

#include "../../coding/file_reader_stream.hpp"
#include "../../coding/file_writer_stream.hpp"
#include "../../coding/sha2.hpp"

#include "../http_request.hpp"
#include "../chunks_download_strategy.hpp"
#include "../platform.hpp"

#include "../../std/scoped_ptr.hpp"
#include "../../std/bind.hpp"

#include <QCoreApplication>

#define TEST_URL1 "http://melnichek.ath.cx:34568/unit_tests/1.txt"
#define TEST_URL_404 "http://melnichek.ath.cx:34568/unit_tests/notexisting_unittest"
#define TEST_URL_PERMANENT "http://melnichek.ath.cx:34568/unit_tests/permanent"
#define TEST_URL_INVALID_HOST "http://melnichek12345.ath.cx"
#define TEST_URL_BIG_FILE "http://melnichek.ath.cx:34568/unit_tests/47kb.file"
#define TEST_URL_HTTPS "https://github.com"
#define TEST_URL_POST "http://melnichek.ath.cx:34568/unit_tests/post.php"

using namespace downloader;


class DownloadObserver
{
  bool m_progressWasCalled;
  HttpRequest::StatusT * m_status;

public:
  DownloadObserver() : m_status(0)
  {
    Reset();
    my::g_LogLevel = LDEBUG;
  }

  void Reset()
  {
    m_progressWasCalled = false;
    if (m_status)
      delete m_status;
    m_status = 0;
  }

  void TestOk()
  {
    TEST(m_progressWasCalled, ("Download progress wasn't called"));
    TEST_NOT_EQUAL(m_status, 0, ());
    TEST_EQUAL(*m_status, HttpRequest::ECompleted, ());
  }

  void TestFailed()
  {
    TEST_NOT_EQUAL(m_status, 0, ());
    TEST_EQUAL(*m_status, HttpRequest::EFailed, ());
  }

  void OnDownloadProgress(HttpRequest & request)
  {
    m_progressWasCalled = true;
    TEST_EQUAL(request.Status(), HttpRequest::EInProgress, ());
  }

  void OnDownloadFinish(HttpRequest & request)
  {
    TEST_EQUAL(m_status, 0, ());
    m_status = new HttpRequest::StatusT(request.Status());
    TEST(*m_status == HttpRequest::EFailed || *m_status == HttpRequest::ECompleted, ());
    QCoreApplication::quit();
  }

};

struct CancelDownload
{
  void OnProgress(HttpRequest & request)
  {
    TEST_GREATER(request.Data().size(), 0, ());
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
    TEST_GREATER(request.Data().size(), 0, ());
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
  HttpRequest::CallbackT onFinish = bind(&DownloadObserver::OnDownloadFinish, &observer, _1);
  HttpRequest::CallbackT onProgress = bind(&DownloadObserver::OnDownloadProgress, &observer, _1);
  { // simple success case
    scoped_ptr<HttpRequest> request(HttpRequest::Get(TEST_URL1, onFinish, onProgress));
    // wait until download is finished
    QCoreApplication::exec();
    observer.TestOk();
    TEST_EQUAL(request->Data(), "Test1", ());
  }

  observer.Reset();
  { // permanent redirect success case
    scoped_ptr<HttpRequest> request(HttpRequest::Get(TEST_URL_PERMANENT, onFinish, onProgress));
    QCoreApplication::exec();
    observer.TestOk();
    TEST_EQUAL(request->Data(), "Test1", ());
  }

  observer.Reset();
  { // fail case 404
    scoped_ptr<HttpRequest> request(HttpRequest::Get(TEST_URL_404, onFinish, onProgress));
    QCoreApplication::exec();
    observer.TestFailed();
    TEST_EQUAL(request->Data().size(), 0, (request->Data()));
  }

  observer.Reset();
  { // fail case not existing host
    scoped_ptr<HttpRequest> request(HttpRequest::Get(TEST_URL_INVALID_HOST, onFinish, onProgress));
    QCoreApplication::exec();
    observer.TestFailed();
    TEST_EQUAL(request->Data().size(), 0, (request->Data()));
  }

  { // cancel download in the middle of the progress
    CancelDownload canceler;
    /// should be deleted in canceler
    HttpRequest::Get(TEST_URL_BIG_FILE,
        bind(&CancelDownload::OnFinish, &canceler, _1),
        bind(&CancelDownload::OnProgress, &canceler, _1));
    QCoreApplication::exec();
  }

  observer.Reset();
  { // https success case
    scoped_ptr<HttpRequest> request(HttpRequest::Get(TEST_URL_HTTPS, onFinish, onProgress));
    // wait until download is finished
    QCoreApplication::exec();
    observer.TestOk();
    TEST_GREATER(request->Data().size(), 0, ());
  }

  { // Delete request at the end of successful download
    DeleteOnFinish deleter;
    /// should be deleted in deleter on finish
    HttpRequest::Get(TEST_URL1,
        bind(&DeleteOnFinish::OnFinish, &deleter, _1),
        bind(&DeleteOnFinish::OnProgress, &deleter, _1));
    QCoreApplication::exec();
  }
}

UNIT_TEST(DownloaderSimplePost)
{
  DownloadObserver observer;
  HttpRequest::CallbackT onFinish = bind(&DownloadObserver::OnDownloadFinish, &observer, _1);
  HttpRequest::CallbackT onProgress = bind(&DownloadObserver::OnDownloadProgress, &observer, _1);
  { // simple success case
    string const postData = "{\"jsonKey\":\"jsonValue\"}";
    scoped_ptr<HttpRequest> request(HttpRequest::PostJson(TEST_URL_POST, postData, onFinish, onProgress));
    // wait until download is finished
    QCoreApplication::exec();
    observer.TestOk();
    TEST_EQUAL(request->Data(), postData, ());
  }
}

UNIT_TEST(ChunksDownloadStrategy)
{
  string const S1 = "UrlOfServer1";
  string const S2 = "UrlOfServer2";
  string const S3 = "UrlOfServer3";
  ChunksDownloadStrategy::RangeT const R1(0, 249), R2(250, 499), R3(500, 749), R4(750, 800);
  vector<string> servers;
  servers.push_back(S1);
  servers.push_back(S2);
  servers.push_back(S3);
  int64_t const FILE_SIZE = 800;
  int64_t const CHUNK_SIZE = 250;
  ChunksDownloadStrategy strategy(servers, FILE_SIZE, CHUNK_SIZE);

  string s1;
  ChunksDownloadStrategy::RangeT r1;
  TEST_EQUAL(strategy.NextChunk(s1, r1), ChunksDownloadStrategy::ENextChunk, ());

  string s2;
  ChunksDownloadStrategy::RangeT r2;
  TEST_EQUAL(strategy.NextChunk(s2, r2), ChunksDownloadStrategy::ENextChunk, ());

  string s3;
  ChunksDownloadStrategy::RangeT r3;
  TEST_EQUAL(strategy.NextChunk(s3, r3), ChunksDownloadStrategy::ENextChunk, ());

  string sEmpty;
  ChunksDownloadStrategy::RangeT rEmpty;
  TEST_EQUAL(strategy.NextChunk(sEmpty, rEmpty), ChunksDownloadStrategy::ENoFreeServers, ());
  TEST_EQUAL(strategy.NextChunk(sEmpty, rEmpty), ChunksDownloadStrategy::ENoFreeServers, ());

  TEST(s1 != s2 && s2 != s3 && s3 != s1, (s1, s2, s3));

  TEST(r1 != r2 && r2 != r3 && r3 != r1, (r1, r2, r3));
  TEST(r1 == R1 || r1 == R2 || r1 == R3 || r1 == R4, (r1));
  TEST(r2 == R1 || r2 == R2 || r2 == R3 || r2 == R4, (r2));
  TEST(r3 == R1 || r3 == R2 || r3 == R3 || r3 == R4, (r3));

  strategy.ChunkFinished(true, r1);

  string s4;
  ChunksDownloadStrategy::RangeT r4;
  TEST_EQUAL(strategy.NextChunk(s4, r4), ChunksDownloadStrategy::ENextChunk, ());
  TEST_EQUAL(s4, s1, ());
  TEST(r4 != r1 && r4 != r2 && r4 != r3, (r4));

  TEST_EQUAL(strategy.NextChunk(sEmpty, rEmpty), ChunksDownloadStrategy::ENoFreeServers, ());
  TEST_EQUAL(strategy.NextChunk(sEmpty, rEmpty), ChunksDownloadStrategy::ENoFreeServers, ());

  strategy.ChunkFinished(false, r2);

  TEST_EQUAL(strategy.NextChunk(sEmpty, rEmpty), ChunksDownloadStrategy::ENoFreeServers, ());

  strategy.ChunkFinished(true, r4);

  string s5;
  ChunksDownloadStrategy::RangeT r5;
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
  string const S1 = "UrlOfServer1";
  string const S2 = "UrlOfServer2";
  ChunksDownloadStrategy::RangeT const R1(0, 249), R2(250, 499), R3(500, 749), R4(750, 800);
  vector<string> servers;
  servers.push_back(S1);
  servers.push_back(S2);
  int64_t const FILE_SIZE = 800;
  int64_t const CHUNK_SIZE = 250;
  ChunksDownloadStrategy strategy(servers, FILE_SIZE, CHUNK_SIZE);

  string s1;
  ChunksDownloadStrategy::RangeT r1;
  TEST_EQUAL(strategy.NextChunk(s1, r1), ChunksDownloadStrategy::ENextChunk, ());
  string s2;
  ChunksDownloadStrategy::RangeT r2;
  TEST_EQUAL(strategy.NextChunk(s2, r2), ChunksDownloadStrategy::ENextChunk, ());
  TEST_EQUAL(strategy.NextChunk(s2, r2), ChunksDownloadStrategy::ENoFreeServers, ());

  strategy.ChunkFinished(false, r1);

  TEST_EQUAL(strategy.NextChunk(s2, r2), ChunksDownloadStrategy::ENoFreeServers, ());

  strategy.ChunkFinished(false, r2);

  TEST_EQUAL(strategy.NextChunk(s2, r2), ChunksDownloadStrategy::EDownloadFailed, ());
}

bool ReadFileAsString(string const & file, string & outStr)
{
  try
  {
    FileReader f(file);
    f.ReadAsString(outStr);
  }
  catch (FileReader::Exception const &)
  {
    return false;
  }
  return true;
}

UNIT_TEST(DownloadChunks)
{
  string const FILENAME = "some_downloader_test_file";

  { // remove data from previously failed files
    Platform::FilesList files;
    Platform::GetFilesInDir(".", "*.resume", files);
    Platform::GetFilesInDir(".", "*.downloading", files);
    for (Platform::FilesList::iterator it = files.begin(); it != files.end(); ++it)
      FileWriter::DeleteFileX(*it);
  }

  DownloadObserver observer;
  HttpRequest::CallbackT onFinish = bind(&DownloadObserver::OnDownloadFinish, &observer, _1);
  HttpRequest::CallbackT onProgress = bind(&DownloadObserver::OnDownloadProgress, &observer, _1);

  vector<string> urls;
  urls.push_back(TEST_URL1);
  urls.push_back(TEST_URL1);
  int64_t FILESIZE = 5;

  { // should use only one thread
    scoped_ptr<HttpRequest> request(HttpRequest::GetFile(urls, FILENAME, FILESIZE,
                                                           onFinish, onProgress));
    // wait until download is finished
    QCoreApplication::exec();
    observer.TestOk();
    TEST_EQUAL(request->Data(), FILENAME, ());
    string s;
    TEST(ReadFileAsString(FILENAME, s), ());
    TEST_EQUAL(s, "Test1", ());
    FileWriter::DeleteFileX(FILENAME);
    uint64_t size;
    TEST(!Platform::GetFileSizeByFullPath(FILENAME + ".resume", size), ("No resume file on success"));
  }

  observer.Reset();

  urls.clear();
  urls.push_back(TEST_URL_BIG_FILE);
  urls.push_back(TEST_URL_BIG_FILE);
  urls.push_back(TEST_URL_BIG_FILE);
  FILESIZE = 5;

  { // 3 threads - fail, because of invalid size
    scoped_ptr<HttpRequest> request(HttpRequest::GetFile(urls, FILENAME, FILESIZE,
                                                           onFinish, onProgress, 2048));
    // wait until download is finished
    QCoreApplication::exec();
    observer.TestFailed();
    FileWriter::DeleteFileX(FILENAME);
    uint64_t size;
    TEST(Platform::GetFileSizeByFullPath(FILENAME + ".resume", size), ("Resume file should present"));
    FileWriter::DeleteFileX(FILENAME + ".resume");
  }

  string const SHA256 = "EE6AE6A2A3619B2F4A397326BEC32583DE2196D9D575D66786CB3B6F9D04A633";

  observer.Reset();

  urls.clear();
  urls.push_back(TEST_URL_BIG_FILE);
  urls.push_back(TEST_URL_BIG_FILE);
  urls.push_back(TEST_URL_BIG_FILE);
  FILESIZE = 47684;

  { // 3 threads - succeeded
    scoped_ptr<HttpRequest> request(HttpRequest::GetFile(urls, FILENAME, FILESIZE,
                                                           onFinish, onProgress, 2048));
    // wait until download is finished
    QCoreApplication::exec();
    observer.TestOk();
    string s;
    TEST(ReadFileAsString(FILENAME, s), ());
    TEST_EQUAL(sha2::digest256(s), SHA256, ());
    FileWriter::DeleteFileX(FILENAME);
    uint64_t size;
    TEST(!Platform::GetFileSizeByFullPath(FILENAME + ".resume", size), ("No resume file on success"));
  }

  observer.Reset();

  urls.clear();
  urls.push_back(TEST_URL_BIG_FILE);
  urls.push_back(TEST_URL1);
  urls.push_back(TEST_URL_404);
  FILESIZE = 47684;

  { // 3 threads with only one valid url - succeeded
    scoped_ptr<HttpRequest> request(HttpRequest::GetFile(urls, FILENAME, FILESIZE, onFinish, onProgress, 2048));
    // wait until download is finished
    QCoreApplication::exec();
    observer.TestOk();
    string s;
    TEST(ReadFileAsString(FILENAME, s), ());
    TEST_EQUAL(sha2::digest256(s), SHA256, ());
    FileWriter::DeleteFileX(FILENAME);
    uint64_t size;
    TEST(!Platform::GetFileSizeByFullPath(FILENAME + ".resume", size), ("No resume file on success"));
  }

  observer.Reset();

  urls.clear();
  urls.push_back(TEST_URL_BIG_FILE);
  urls.push_back(TEST_URL_BIG_FILE);
  FILESIZE = 12345;

  { // 2 threads and all points to file with invalid size - fail
    scoped_ptr<HttpRequest> request(HttpRequest::GetFile(urls, FILENAME, FILESIZE, onFinish, onProgress, 2048));
    // wait until download is finished
    QCoreApplication::exec();
    observer.TestFailed();
    FileWriter::DeleteFileX(FILENAME);
    uint64_t size;
    TEST(Platform::GetFileSizeByFullPath(FILENAME + ".resume", size), ("Resume file should present"));
    FileWriter::DeleteFileX(FILENAME + ".resume");
  }
}


int64_t FILESIZE = 47684;
int64_t const beg1 = 123, end1 = 1230, beg2 = 44000, end2 = 47683;

struct ResumeChecker
{
  size_t m_counter;
  ResumeChecker() : m_counter(0) {}
  void OnProgress(HttpRequest & request)
  {
    if (m_counter == 0)
    {
      TEST_EQUAL(request.Progress(), make_pair(beg2 + 1, FILESIZE), ());
    }
    else if (m_counter == 1)
    {
      TEST_EQUAL(request.Progress(), make_pair(FILESIZE, FILESIZE), ());
    }
    else
    {
      TEST(false, ("Progress should be called exactly 2 times"));
    }
    ++m_counter;
  }
  void OnFinish(HttpRequest & request)
  {
    TEST_EQUAL(request.Status(), HttpRequest::ECompleted, ());
    QCoreApplication::exit();
  }
};

UNIT_TEST(DownloadResumeChunks)
{
  string const FILENAME = "some_test_filename_12345";
  string const RESUME_FILENAME = FILENAME + ".resume";
  string const SHA256 = "EE6AE6A2A3619B2F4A397326BEC32583DE2196D9D575D66786CB3B6F9D04A633";

  { // remove data from previously failed files
    Platform::FilesList files;
    Platform::GetFilesInDir(".", "*.resume", files);
    Platform::GetFilesInDir(".", "*.downloading", files);
    for (Platform::FilesList::iterator it = files.begin(); it != files.end(); ++it)
      FileWriter::DeleteFileX(*it);
  }

  vector<string> urls;
  urls.push_back(TEST_URL_BIG_FILE);

  // 1st step - download full file
  {
    DownloadObserver observer;

    scoped_ptr<HttpRequest> request(HttpRequest::GetFile(urls, FILENAME, FILESIZE,
                            bind(&DownloadObserver::OnDownloadFinish, &observer, _1),
                            bind(&DownloadObserver::OnDownloadProgress, &observer, _1)));
    QCoreApplication::exec();
    observer.TestOk();
    string s;
    TEST(ReadFileAsString(FILENAME, s), ());
    TEST_EQUAL(sha2::digest256(s), SHA256, ());
  }
  // 2nd step - mark some file blocks as not downloaded
  {
    FileWriter f(FILENAME, FileWriter::OP_WRITE_EXISTING);
    f.Seek(beg1);
    char b1[end1 - beg1];
    for (size_t i = 0; i < ARRAY_SIZE(b1); ++i) b1[i] = 0;
    f.Write(b1, ARRAY_SIZE(b1));

    f.Seek(beg2);
    char b2[end2 - beg2];
    for (size_t i = 0; i < ARRAY_SIZE(b2); ++i) b2[i] = 0;
    f.Write(b2, ARRAY_SIZE(b2));

    vector<pair<int64_t, int64_t> > originalRanges;
    originalRanges.push_back(make_pair(beg1, end1));
    originalRanges.push_back(make_pair(beg2, end2));
    {
      FileWriterStream fws(RESUME_FILENAME);
      fws << originalRanges;
    }
    // check if ranges are stored correctly
    vector<pair<int64_t, int64_t> > loadedRanges;
    {
      FileReaderStream frs(RESUME_FILENAME);
      frs >> loadedRanges;
    }
    TEST_EQUAL(originalRanges, loadedRanges, ());
  }
  // 3rd step - check that resume works
  {
    ResumeChecker checker;
    scoped_ptr<HttpRequest> request(HttpRequest::GetFile(urls, FILENAME, FILESIZE,
                                                         bind(&ResumeChecker::OnFinish, &checker, _1),
                                                         bind(&ResumeChecker::OnProgress, &checker, _1)));
    QCoreApplication::exec();
    string s;
    TEST(ReadFileAsString(FILENAME, s), ());
    TEST_EQUAL(sha2::digest256(s), SHA256, ());
    uint64_t size = 0;
    TEST(!GetPlatform().GetFileSizeByFullPath(RESUME_FILENAME, size), ());
    FileWriter::DeleteFileX(FILENAME);
  }
}

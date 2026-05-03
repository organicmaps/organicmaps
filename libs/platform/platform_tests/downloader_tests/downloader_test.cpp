#include "testing/testing.hpp"

#include "platform/chunks_download_strategy.hpp"
#include "platform/http_request.hpp"
#include "platform/platform.hpp"

#include "coding/file_reader.hpp"
#include "coding/file_writer.hpp"
#include "coding/internal/file_data.hpp"

#include "base/logging.hpp"
#include "base/std_serialization.hpp"

#include <QtCore/QCoreApplication>

#include <functional>
#include <memory>
#include <string_view>
#include <vector>

#include "defines.hpp"

namespace downloader_test
{
using namespace downloader;
using namespace std::placeholders;
using std::bind, std::string, std::string_view, std::vector;

char constexpr kTestUrl1[] = "http://localhost:24568/unit_tests/1.txt";
char constexpr kTestUrl404[] = "http://localhost:24568/unit_tests/notexisting_unittest";
char constexpr kTestUrlBigFile[] = "http://localhost:24568/unit_tests/47kb.file";

// Should match file size in tools/python/ResponseProvider.py
int constexpr kBigFileSize = 47684;

// RAII helper: gives each test a unique tmp path and wipes the result/.downloading/.resume
// triple at construction (clean slate) and destruction (no leakage to next test, even on
// assertion abort during stack unwind).
struct ScopedDownloadArtifacts
{
  string const m_path;

  explicit ScopedDownloadArtifacts(string_view prefix) : m_path(GetPlatform().TmpPathForFile(string(prefix), ".tmp"))
  {
    Wipe();
  }

  ~ScopedDownloadArtifacts() { Wipe(); }

  ScopedDownloadArtifacts(ScopedDownloadArtifacts const &) = delete;
  ScopedDownloadArtifacts & operator=(ScopedDownloadArtifacts const &) = delete;

  string Downloading() const { return m_path + DOWNLOADING_FILE_EXTENSION; }
  string Resume() const { return m_path + RESUME_FILE_EXTENSION; }

private:
  void Wipe() const
  {
    Platform::RemoveFileIfExists(m_path);
    Platform::RemoveFileIfExists(Downloading());
    Platform::RemoveFileIfExists(Resume());
  }
};

class DownloadObserver
{
  bool m_progressWasCalled;
  // Chunked downloads can return one status per chunk (thread).
  vector<DownloadStatus> m_statuses;
  // Interrupt download after this number of chunks
  int m_chunksToFail;
  base::ScopedLogLevelChanger const m_debugLogLevel;

public:
  DownloadObserver() : m_chunksToFail(-1), m_debugLogLevel(LDEBUG) { Reset(); }

  void CancelDownloadOnGivenChunk(int chunksToFail) { m_chunksToFail = chunksToFail; }

  void Reset()
  {
    m_progressWasCalled = false;
    m_statuses.clear();
    m_chunksToFail = -1;
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

// Convenience builder for HttpRequest::GetFile bound to a DownloadObserver. Two overloads:
// the multi-URL form for tests that need URL fallback semantics, and a single-URL form
// defaulting to kTestUrlBigFile (the common case).
std::unique_ptr<HttpRequest> MakeRequest(vector<string> const & urls, string const & file, int64_t fileSize,
                                         DownloadObserver & obs, bool doCleanOnCancel = true, int64_t chunkSize = 1024)
{
  return std::unique_ptr<HttpRequest>(
      HttpRequest::GetFile(urls, file, fileSize, bind(&DownloadObserver::OnDownloadFinish, &obs, _1),
                           bind(&DownloadObserver::OnDownloadProgress, &obs, _1), doCleanOnCancel, chunkSize));
}

std::unique_ptr<HttpRequest> MakeRequest(string const & file, int64_t fileSize, DownloadObserver & obs,
                                         bool doCleanOnCancel = true, int64_t chunkSize = 1024)
{
  return MakeRequest({kTestUrlBigFile}, file, fileSize, obs, doCleanOnCancel, chunkSize);
}

// Reads .resume via the same load path the production constructor uses (LoadOrInitChunks).
// Returns the bytes-completed claim recorded in the resume file.
int64_t LoadResumeBytes(string const & resumeFile, int64_t fileSize, int64_t chunkSize)
{
  ChunksDownloadStrategy s({});
  return s.LoadOrInitChunks(resumeFile, fileSize, chunkSize);
}

UNIT_TEST(ChunksDownloadStrategy)
{
  vector<string> const servers = {"UrlOfServer1", "UrlOfServer2", "UrlOfServer3"};

  typedef std::pair<int64_t, int64_t> RangeT;
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

  typedef std::pair<int64_t, int64_t> RangeT;

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
    return {};
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
}  // namespace

UNIT_TEST(DownloadChunks)
{
  ScopedDownloadArtifacts files{"downloader_test_"};

  DownloadObserver observer;

  {
    // should use only one thread
    auto const request = MakeRequest({kTestUrl1, kTestUrl1}, files.m_path, 5, observer,
                                     /*doCleanOnCancel=*/true, /*chunkSize=*/512 * 1024);
    QCoreApplication::exec();
    observer.TestOk();
    TEST_EQUAL(request->GetFilePath(), files.m_path, ());
    TEST_EQUAL(ReadFileAsString(files.m_path), "Test1", ());
    FinishDownloadSuccess(files.m_path);
  }

  observer.Reset();
  {
    // 3 threads - fail, because of invalid size
    auto const request = MakeRequest({kTestUrlBigFile, kTestUrlBigFile, kTestUrlBigFile}, files.m_path, 5, observer,
                                     /*doCleanOnCancel=*/true, /*chunkSize=*/2048);
    QCoreApplication::exec();
    observer.TestFailed();
    FinishDownloadFail(files.m_path);
  }

  observer.Reset();
  {
    // 3 threads - succeeded
    auto const request =
        MakeRequest({kTestUrlBigFile, kTestUrlBigFile, kTestUrlBigFile}, files.m_path, kBigFileSize, observer,
                    /*doCleanOnCancel=*/true, /*chunkSize=*/2048);
    QCoreApplication::exec();
    observer.TestOk();
    FinishDownloadSuccess(files.m_path);
  }

  observer.Reset();
  {
    // 3 threads with only one valid url - succeeded
    auto const request = MakeRequest({kTestUrlBigFile, kTestUrl1, kTestUrl404}, files.m_path, kBigFileSize, observer,
                                     /*doCleanOnCancel=*/true, /*chunkSize=*/2048);
    QCoreApplication::exec();
    observer.TestOk();
    FinishDownloadSuccess(files.m_path);
  }

  observer.Reset();
  {
    // 2 threads and all points to file with invalid size - fail
    auto const request = MakeRequest({kTestUrlBigFile, kTestUrlBigFile}, files.m_path, 12345, observer,
                                     /*doCleanOnCancel=*/true, /*chunkSize=*/2048);
    QCoreApplication::exec();
    observer.TestFailed();
    FinishDownloadFail(files.m_path);
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
  ScopedDownloadArtifacts files{"downloader_resume_test_"};

  // 1st step - download full file
  {
    DownloadObserver observer;
    auto const request = MakeRequest(files.m_path, kBigFileSize, observer);
    QCoreApplication::exec();
    observer.TestOk();

    uint64_t size;
    TEST(!base::GetFileSize(files.Resume(), size), ("No resume file on success"));
  }

  // 2nd step - mark some file blocks as not downloaded
  {
    // to substitute temporary not fully downloaded file
    TEST(base::RenameFileX(files.m_path, files.Downloading()), ());

    FileWriter f(files.Downloading(), FileWriter::OP_WRITE_EXISTING);
    f.Seek(beg1);
    char b1[end1 - beg1 + 1] = {0};
    f.Write(b1, ARRAY_SIZE(b1));

    f.Seek(beg2);
    char b2[end2 - beg2 + 1] = {0};
    f.Write(b2, ARRAY_SIZE(b2));

    ChunksDownloadStrategy strategy((vector<string>()));
    strategy.AddChunk(std::make_pair(int64_t(0), beg1 - 1), ChunksDownloadStrategy::CHUNK_COMPLETE);
    strategy.AddChunk(std::make_pair(beg1, end1), ChunksDownloadStrategy::CHUNK_FREE);
    strategy.AddChunk(std::make_pair(end1 + 1, beg2 - 1), ChunksDownloadStrategy::CHUNK_COMPLETE);
    strategy.AddChunk(std::make_pair(beg2, end2), ChunksDownloadStrategy::CHUNK_FREE);

    strategy.SaveChunks(kBigFileSize, files.Resume());
  }

  // 3rd step - check that resume works
  {
    ResumeChecker checker;
    std::unique_ptr<HttpRequest> const request(HttpRequest::GetFile({kTestUrlBigFile}, files.m_path, kBigFileSize,
                                                                    bind(&ResumeChecker::OnFinish, &checker, _1),
                                                                    bind(&ResumeChecker::OnProgress, &checker, _1)));
    QCoreApplication::exec();

    FinishDownloadSuccess(files.m_path);
  }
}

// Cancel mid-flight with doCleanOnCancel=true (the "user removes country" path).
// Asserts: ~FileHttpRequest wipes both .downloading and .resume when destroyed while
// the request is still InProgress. No other test exercises this branch — DownloadChunks
// always lets requests complete naturally.
UNIT_TEST(HttpRequest_CancelMidFlight_DoClean_WipesArtifacts)
{
  ScopedDownloadArtifacts files{"http_cancel_clean_"};
  DownloadObserver obs;
  // Cancel after the periodic SaveResumeChunks() has fired, otherwise no .resume exists
  // on disk at dtor time and the wipe assertion below is vacuous.
  int constexpr kCancelAtChunk = kPeriodicResumeSaveInterval + 1;
  obs.CancelDownloadOnGivenChunk(kCancelAtChunk);
  {
    auto req = MakeRequest(files.m_path, kBigFileSize, obs, /*doCleanOnCancel=*/true);
    QCoreApplication::exec();
  }  // dtor runs while m_status == InProgress

  uint64_t size = 0;
  TEST(!base::GetFileSize(files.Downloading(), size), ("Downloading must be wiped"));
  TEST(!base::GetFileSize(files.Resume(), size), ("Resume must be wiped"));
}

// Cancel mid-flight with doCleanOnCancel=false (the "graceful shutdown" path).
// Asserts: ~FileHttpRequest preserves both files when destroyed while the request is
// still InProgress. Pins down the positive contract of the new flag semantics.
UNIT_TEST(HttpRequest_CancelMidFlight_NoClean_PreservesArtifacts)
{
  ScopedDownloadArtifacts files{"http_cancel_preserve_"};
  DownloadObserver obs;
  obs.CancelDownloadOnGivenChunk(5);
  {
    auto req = MakeRequest(files.m_path, kBigFileSize, obs, /*doCleanOnCancel=*/false);
    QCoreApplication::exec();
  }
  uint64_t downloadingSize = 0, resumeSize = 0;
  TEST(base::GetFileSize(files.Downloading(), downloadingSize), ("Downloading must be preserved"));
  TEST(base::GetFileSize(files.Resume(), resumeSize), ("Resume must be preserved"));
  TEST_GREATER(downloadingSize, 0u, ());
  TEST_GREATER(resumeSize, 0u, ());
}

// Regression test for the destructor flush. Cancel point must stay below
// kPeriodicResumeSaveInterval so the periodic save never fires; without the dtor's
// SaveResumeChunks() call, .resume would be missing or stale.
UNIT_TEST(HttpRequest_CancelMidFlight_NoClean_ResumeReflectsAllCompletedChunks)
{
  ScopedDownloadArtifacts files{"http_resume_flush_"};
  int64_t constexpr kChunkSize = 1024;
  int constexpr kCancelAtChunk = 5;
  static_assert(kCancelAtChunk < kPeriodicResumeSaveInterval, "Periodic save would mask the dtor flush");
  DownloadObserver obs;
  obs.CancelDownloadOnGivenChunk(kCancelAtChunk);
  {
    auto req = MakeRequest(files.m_path, kBigFileSize, obs, /*doCleanOnCancel=*/false, kChunkSize);
    QCoreApplication::exec();
  }
  // OnDownloadProgress fires after ChunkFinished marks the chunk complete in the strategy,
  // so the cancel-triggering chunk is committed by quit() time. The next chunk (started
  // synchronously after the callback) may or may not finish before exec() returns, so the
  // bound is >= kCancelAtChunk, never == .
  int64_t const resumed = LoadResumeBytes(files.Resume(), kBigFileSize, kChunkSize);
  TEST_GREATER_OR_EQUAL(resumed, kCancelAtChunk * kChunkSize,
                        ("Resume file under-reports completed bytes; dtor flush regressed"));
}

// End-to-end round-trip: cancel + preserve + reload + complete. Validates the user-facing
// claim that graceful shutdown does not lose progress. Catches regressions in the dtor
// preservation/flush, ctor reload via LoadOrInitChunks, and success cleanup, all in one test.
UNIT_TEST(HttpRequest_ResumeFromPreservedArtifacts_SkipsCompletedChunks)
{
  ScopedDownloadArtifacts files{"http_round_trip_"};
  int64_t constexpr kChunkSize = 1024;
  int64_t bytesAfterCancel = 0;
  {
    DownloadObserver obs;
    obs.CancelDownloadOnGivenChunk(5);
    auto req = MakeRequest(files.m_path, kBigFileSize, obs, /*doCleanOnCancel=*/false, kChunkSize);
    QCoreApplication::exec();
    bytesAfterCancel = req->GetProgress().m_bytesDownloaded;
  }
  TEST_GREATER(bytesAfterCancel, 0, ());

  DownloadObserver obs2;
  auto req2 = MakeRequest(files.m_path, kBigFileSize, obs2, /*doCleanOnCancel=*/false, kChunkSize);
  // Construction reloaded preserved resume state — this is the user-facing claim.
  TEST_GREATER_OR_EQUAL(req2->GetProgress().m_bytesDownloaded, bytesAfterCancel, ());
  QCoreApplication::exec();
  obs2.TestOk();

  // After natural success, files must be cleaned up regardless of the doCleanOnCancel flag.
  uint64_t size = 0;
  TEST(base::GetFileSize(files.m_path, size), ("Result file must exist"));
  TEST(!base::GetFileSize(files.Downloading(), size), ());
  TEST(!base::GetFileSize(files.Resume(), size), ());
}

// On success, the OnChunkFinished cleanup path must run regardless of doCleanOnCancel —
// the flag only governs the dtor's InProgress branch. Pins this invariant explicitly so
// a future "optimization" gating success cleanup behind the flag is caught.
UNIT_TEST(HttpRequest_Success_AlwaysCleansArtifacts_DespiteNoClean)
{
  ScopedDownloadArtifacts files{"http_success_noclean_"};
  DownloadObserver obs;
  {
    auto req = MakeRequest(files.m_path, kBigFileSize, obs, /*doCleanOnCancel=*/false);
    QCoreApplication::exec();
  }
  obs.TestOk();

  uint64_t size = 0;
  TEST(base::GetFileSize(files.m_path, size), ("Result must exist"));
  TEST(!base::GetFileSize(files.Downloading(), size), ("Downloading wiped on success"));
  TEST(!base::GetFileSize(files.Resume(), size), ("Resume wiped on success"));
}

// On failure, OnChunkFinished's save-on-fail path must run regardless of doCleanOnCancel.
// Existing DownloadChunks failure tests only cover the flag=true case; this exercises both.
UNIT_TEST(HttpRequest_Failure_PreservesResume_RegardlessOfFlag)
{
  for (bool doCleanOnCancel : {true, false})
  {
    ScopedDownloadArtifacts files{"http_fail_"};
    DownloadObserver obs;
    {
      // fileSize mismatch with the real server file triggers Failed status.
      auto req = MakeRequest({kTestUrlBigFile, kTestUrlBigFile}, files.m_path, /*fileSize=*/12345, obs, doCleanOnCancel,
                             /*chunkSize=*/2048);
      QCoreApplication::exec();
    }
    obs.TestFailed();

    uint64_t size = 0;
    TEST(base::GetFileSize(files.Resume(), size), ("Resume must be preserved on failure"));
  }
}

}  // namespace downloader_test

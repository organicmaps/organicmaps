#include "../../testing/testing.hpp"

#include "../platform.hpp"
#include "../download_manager.hpp"

#include "../../coding/file_writer.hpp"
#include "../../coding/file_reader.hpp"

#include "../../std/bind.hpp"

#include <QCoreApplication>

// file on the server contains "Test1"
#define TEST_FILE_URL1 "http://melnichek.ath.cx:34568/unit_tests/1.txt"
#define TEST_FILE_NAME1 "unit_test1.tmp"
// file on the server contains "Test2"
#define TEST_FILE_URL2 "http://melnichek.ath.cx:34568/unit_tests/2.txt"
#define TEST_FILE_NAME2 "unit_test2.tmp"
// file on the server contains "Test3"
#define TEST_FILE_URL3 "http://melnichek.ath.cx:34568/unit_tests/3.txt"
#define TEST_FILE_NAME3 "unit_test3.tmp"

#define TEST_INVALID_URL "http://very_invalid_url.kotorogo.net/okak.test"

#define TEST_ABSENT_FILE_URL "http://melnichek.ath.cx:34568/unit_tests/not_existing_file"
#define TEST_ABSENT_FILE_NAME "not_existing_file"

#define TEST_LOCKED_FILE_URL  "http://melnichek.ath.cx:34568/unit_tests/1.txt"
#define TEST_LOCKED_FILE_NAME "locked_file.tmp"

#define TEST_BIG_FILE_URL "http://melnichek.ath.cx:34568/unit_tests/47kb.file"

Platform & gPlatform = GetPlatform();
DownloadManager & gMgr = GetDownloadManager();

string GetHostFromUrl(string const & url)
{
  size_t start = url.find("://");
  if (start != string::npos)
    start += 3;
  else
    start = 0;
  size_t end = url.find('/', start);
  if (end == string::npos)
    end = url.size();
  return url.substr(start, end - start);
}

template<int TMaxDownloadsNum>
struct DlObserver
{
  size_t m_downloadsProcessed;
  HttpFinishedParams m_result[TMaxDownloadsNum];

  string m_progressUrl;

  DlObserver() : m_downloadsProcessed(0)
  {
    for (size_t i = 0; i < TMaxDownloadsNum; ++i)
      m_result[i].m_error = EHttpDownloadFailed;
  }

  void OnDownloadFinished(HttpFinishedParams const & result)
  {
    m_result[m_downloadsProcessed++] = result;
    if (m_downloadsProcessed >= TMaxDownloadsNum)
      QCoreApplication::quit();  // return control to test function body
  }

  void OnDownloadProgress(HttpProgressT const & progress)
  {
    m_progressUrl = progress.m_url;
    if (progress.m_total < 0)
    {
      cerr << "Your hosting doesn't support total file size or invalid resume range was given for "
           << m_progressUrl << endl;
    }
    // for big file - cancel downloading after 40Kb were transferred
    if (progress.m_current > 40 * 1024)
      gMgr.CancelDownload(progress.m_url);
  }
};

int gArgc = 1;
char * gArgv[] = { 0 };

UNIT_TEST(SingleDownload)
{
  size_t const NUM = 1;
  DlObserver<NUM> observer;

  FileWriter::DeleteFileX(TEST_FILE_NAME1);
  HttpStartParams params;
  params.m_url = TEST_FILE_URL1;
  params.m_fileToSave = TEST_FILE_NAME1;
  params.m_finish = bind(&DlObserver<NUM>::OnDownloadFinished, &observer, _1);
  params.m_progress = bind(&DlObserver<NUM>::OnDownloadProgress, &observer, _1);
  gMgr.HttpRequest(params);

  QCoreApplication::exec();

  TEST_EQUAL( observer.m_result[0].m_error, EHttpDownloadOk, ("Do you have internet connection?") );
  TEST( gPlatform.IsFileExists(TEST_FILE_NAME1), () );
  FileWriter::DeleteFileX(TEST_FILE_NAME1);
}

UNIT_TEST(MultiDownload)
{
  size_t const NUM = 3;
  DlObserver<NUM> observer;

  FileWriter::DeleteFileX(TEST_FILE_NAME1);
  FileWriter::DeleteFileX(TEST_FILE_NAME2);
  FileWriter::DeleteFileX(TEST_FILE_NAME3);
  HttpStartParams params;
  params.m_url = TEST_FILE_URL1;
  params.m_fileToSave = TEST_FILE_NAME1;
  params.m_finish = bind(&DlObserver<NUM>::OnDownloadFinished, &observer, _1);
  params.m_progress = bind(&DlObserver<NUM>::OnDownloadProgress, &observer, _1);
  gMgr.HttpRequest(params);
  params.m_url = TEST_FILE_URL2;
  params.m_fileToSave = TEST_FILE_NAME2;
  gMgr.HttpRequest(params);
  params.m_url = TEST_FILE_URL3;
  params.m_fileToSave = TEST_FILE_NAME3;
  gMgr.HttpRequest(params);

  QCoreApplication::exec();

  TEST_EQUAL( observer.m_result[0].m_error, EHttpDownloadOk, ("Do you have internet connection?") );
  TEST( gPlatform.IsFileExists(TEST_FILE_NAME1), () );
  FileWriter::DeleteFileX(TEST_FILE_NAME1);

  TEST_EQUAL( observer.m_result[1].m_error, EHttpDownloadOk, ("Do you have internet connection?") );
  TEST( gPlatform.IsFileExists(TEST_FILE_NAME2), () );
  FileWriter::DeleteFileX(TEST_FILE_NAME2);

  TEST_EQUAL( observer.m_result[2].m_error, EHttpDownloadOk, ("Do you have internet connection?") );
  TEST( gPlatform.IsFileExists(TEST_FILE_NAME3), () );
  FileWriter::DeleteFileX(TEST_FILE_NAME3);
}

UNIT_TEST(InvalidUrl)
{
  size_t const NUM = 1;
  DlObserver<NUM> observer;
  // this should be set to error after executing
  observer.m_result[0].m_error = EHttpDownloadOk;

  HttpStartParams params;
  params.m_url = TEST_INVALID_URL;
  params.m_fileToSave = TEST_FILE_NAME1;
  params.m_finish = bind(&DlObserver<NUM>::OnDownloadFinished, &observer, _1);
  params.m_progress = bind(&DlObserver<NUM>::OnDownloadProgress, &observer, _1);
  gMgr.HttpRequest(params);

  QCoreApplication::exec();

  TEST_EQUAL( observer.m_result[0].m_error, EHttpDownloadFailed, () );

  FileWriter::DeleteFileX(TEST_FILE_NAME1);
}

namespace
{
  void ReadFileToString(char const * fName, string & str)
  {
    FileReader f(fName);
    vector<char> buf;
    buf.resize(f.Size());
    f.Read(0, &buf[0], f.Size());
    str.assign(buf.begin(), buf.end());
  }
}

UNIT_TEST(DownloadFileExists)
{
  size_t const NUM = 1;
  DlObserver<NUM> observer;

  string const fileContent("Some Contents");

  {
    FileWriter f(TEST_FILE_NAME1);
    f.Write(fileContent.c_str(), fileContent.size());
  }

  {
    string str;
    ReadFileToString(TEST_FILE_NAME1, str);
    TEST_EQUAL(fileContent, str, ("Writer or Reader don't work?.."));
  }

  HttpStartParams params;
  params.m_url = TEST_FILE_URL1;
  params.m_fileToSave = TEST_FILE_NAME1;
  params.m_finish = bind(&DlObserver<NUM>::OnDownloadFinished, &observer, _1);
  params.m_progress = bind(&DlObserver<NUM>::OnDownloadProgress, &observer, _1);
  gMgr.HttpRequest(params);

  QCoreApplication::exec();

  TEST_EQUAL( observer.m_result[0].m_error, EHttpDownloadOk, () );

  {
    string str;
    ReadFileToString(TEST_FILE_NAME1, str);
    TEST_NOT_EQUAL(fileContent, str, ("File should be overwritten by download manager"));
  }

  FileWriter::DeleteFileX(TEST_FILE_NAME1);
}

UNIT_TEST(DownloadResume)
{
  size_t const NUM = 1;

  string const fileContent("aLeX");

  {
    FileWriter f(TEST_FILE_NAME1 DOWNLOADING_FILE_EXTENSION);
    f.Write(fileContent.c_str(), fileContent.size());
  }

  DlObserver<NUM> observer1;
  HttpStartParams params;
  params.m_url = TEST_FILE_URL1;
  params.m_fileToSave = TEST_FILE_NAME1;
  params.m_finish = bind(&DlObserver<NUM>::OnDownloadFinished, &observer1, _1);
  params.m_progress = bind(&DlObserver<NUM>::OnDownloadProgress, &observer1, _1);
  gMgr.HttpRequest(params);

  QCoreApplication::exec();

  TEST_EQUAL( observer1.m_result[0].m_error, EHttpDownloadOk, () );

  DlObserver<NUM> observer2;
  params.m_url = TEST_FILE_URL1;
  params.m_fileToSave = TEST_FILE_NAME2;
  params.m_finish = bind(&DlObserver<NUM>::OnDownloadFinished, &observer2, _1);
  params.m_progress = bind(&DlObserver<NUM>::OnDownloadProgress, &observer2, _1);
  gMgr.HttpRequest(params);

  QCoreApplication::exec();

  TEST_EQUAL( observer2.m_result[0].m_error, EHttpDownloadOk, () );

  uint64_t size1 = 4, size2 = 5;
  TEST( GetPlatform().GetFileSize(TEST_FILE_NAME1, size1), ());
  TEST( GetPlatform().GetFileSize(TEST_FILE_NAME2, size2), ());

  TEST_EQUAL(size1, size2, ("Sizes should be equal - no resume was enabled"));

  // resume should append content to 1st file
  {
    FileWriter f(TEST_FILE_NAME1 DOWNLOADING_FILE_EXTENSION);
    f.Write(fileContent.c_str(), fileContent.size());
  }
  DlObserver<NUM> observer3;
  params.m_url = TEST_FILE_URL1;
  params.m_fileToSave = TEST_FILE_NAME1;
  params.m_finish = bind(&DlObserver<NUM>::OnDownloadFinished, &observer3, _1);
  params.m_progress = bind(&DlObserver<NUM>::OnDownloadProgress, &observer3, _1);
  gMgr.HttpRequest(params);

  QCoreApplication::exec();

  TEST_EQUAL( observer3.m_result[0].m_error, EHttpDownloadOk, () );

  TEST( GetPlatform().GetFileSize(TEST_FILE_NAME1, size1), ());
  TEST_EQUAL( size1, size2, () );

  {
    string str;
    ReadFileToString(TEST_FILE_NAME1, str);
    TEST_EQUAL(fileContent, str.substr(0, fileContent.size()), ());
    TEST_NOT_EQUAL(fileContent, str, ());
  }

  FileWriter::DeleteFileX(TEST_FILE_NAME1);
  FileWriter::DeleteFileX(TEST_FILE_NAME2);
}

UNIT_TEST(DownloadAbsentFile)
{
  size_t const NUM = 1;
  DlObserver<NUM> observer;

  HttpStartParams params;
  params.m_url = TEST_ABSENT_FILE_URL;
  params.m_fileToSave = TEST_ABSENT_FILE_NAME;
  params.m_finish = bind(&DlObserver<NUM>::OnDownloadFinished, &observer, _1);
  params.m_progress = bind(&DlObserver<NUM>::OnDownloadProgress, &observer, _1);
  gMgr.HttpRequest(params);

  QCoreApplication::exec();

  TEST_EQUAL( observer.m_result[0].m_error, EHttpDownloadFileNotFound, () );
  TEST( !GetPlatform().IsFileExists(TEST_ABSENT_FILE_NAME), () );

  FileWriter::DeleteFileX(TEST_ABSENT_FILE_NAME);
}

UNIT_TEST(DownloadUsingUrlGenerator)
{
  size_t const NUM = 1;
  DlObserver<NUM> observer;

  string const LOCAL_FILE = "unit_test.txt";

  HttpStartParams params;
  params.m_url = LOCAL_FILE;
  params.m_fileToSave = LOCAL_FILE;
  params.m_finish = bind(&DlObserver<NUM>::OnDownloadFinished, &observer, _1);
  params.m_progress = bind(&DlObserver<NUM>::OnDownloadProgress, &observer, _1);
  gMgr.HttpRequest(params);

  QCoreApplication::exec();

  TEST_NOT_EQUAL( observer.m_result[0].m_error, EHttpDownloadFileNotFound, () );
  TEST( GetPlatform().IsFileExists(LOCAL_FILE), () );

  FileWriter::DeleteFileX(LOCAL_FILE);
}

// only on Windows files are actually locked by system
#ifdef OMIM_OS_WINDOWS
UNIT_TEST(DownloadLockedFile)
{
  {
    size_t const NUM = 1;
    DlObserver<NUM> observer;

    FileWriter lockedFile(TEST_LOCKED_FILE_NAME);
    // check that file is actually exists
    {
      bool exists = true;
      try { FileReader f(TEST_LOCKED_FILE_NAME); }
      catch (Reader::OpenException const &) { exists = false; }
      TEST(exists, ("Locked file wasn't created"));
    }

    HttpStartParams params;
    params.m_url = TEST_LOCKED_FILE_URL;
    params.m_fileToSave = TEST_LOCKED_FILE_NAME;
    params.m_finish = bind(&DlObserver<NUM>::OnDownloadFinished, &observer, _1);
    params.m_progress = bind(&DlObserver<NUM>::OnDownloadProgress, &observer, _1);
    gMgr.HttpRequest(params);

    QCoreApplication::exec();

    TEST_EQUAL( observer.m_result[0].m_error, EHttpDownloadFileIsLocked, () );
  }
  FileWriter::DeleteFileX(GetPlatform().WritablePathForFile(TEST_LOCKED_FILE_NAME));
}
#endif

struct HttpPostCallbackHolder
{
  HttpFinishedParams * m_pResult;
  HttpPostCallbackHolder() : m_pResult(NULL) {}
  ~HttpPostCallbackHolder() { delete m_pResult; }
  void OnHttpPost(HttpFinishedParams const & result)
  {
    m_pResult = new HttpFinishedParams(result);
    QCoreApplication::quit();
  }
};

UNIT_TEST(HttpPost)
{
  HttpPostCallbackHolder cbHolder;
  HttpStartParams params;
  params.m_finish = bind(&HttpPostCallbackHolder::OnHttpPost, &cbHolder, _1);
  params.m_contentType = "application/json";
  params.m_postData = "{\"version\":\"1.1.0\",\"request_address\":true}";
  params.m_url = "http://melnichek.ath.cx:34568/unit_tests/post.php";
  gMgr.HttpRequest(params);

  QCoreApplication::exec();

  TEST( cbHolder.m_pResult, () );
  TEST_EQUAL(cbHolder.m_pResult->m_error, EHttpDownloadOk,
             ("HTTP POST failed with error", cbHolder.m_pResult->m_error));
  TEST_EQUAL(cbHolder.m_pResult->m_data, "HTTP POST is OK",
             ("Server sent invalid POST reply"));
}

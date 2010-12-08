#include "../../testing/testing.hpp"

#include "../platform.hpp"
#include "../download_manager.hpp"
#include "../../coding/file_writer.hpp"
#include "../../coding/file_reader.hpp"

#include <QCoreApplication>
#include <boost/bind.hpp>

#include "../../base/start_mem_debug.hpp"

// file on the server contains "Test1"
#define TEST_FILE_URL1 "http://mapswithme.com/unit_tests/1.txt"
#define TEST_FILE_NAME1 "unit_test1.tmp"
// file on the server contains "Test2"
#define TEST_FILE_URL2 "http://mapswithme.com/unit_tests/2.txt"
#define TEST_FILE_NAME2 "unit_test2.tmp"
// file on the server contains "Test3"
#define TEST_FILE_URL3 "http://mapswithme.com/unit_tests/3.txt"
#define TEST_FILE_NAME3 "unit_test3.tmp"

#define TEST_INVALID_URL "http://very_invalid_url.kotorogo.net/okak.test"

#define TEST_BIG_FILE_URL "http://mapswithme.com/unit_tests/47kb.file"

int gArgc = 1;
char * gArgv[] = { 0 };
QCoreApplication gQtApplication(gArgc, gArgv);
#define WAIT_FOR_ASYNC_DOWNLOAD gQtApplication.exec()
#define STOP_WAITING_FOR_ASYNC_DOWNLOAD gQtApplication.exit()
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
  bool m_result[TMaxDownloadsNum];
  string m_url[TMaxDownloadsNum];

  string m_progressUrl;

  DlObserver() : m_downloadsProcessed(0)
  {
    for (size_t i = 0; i < TMaxDownloadsNum; ++i)
      m_result[i] = false;
  }

  void OnDownloadFinished(char const * url, bool successfully)
  {
    ASSERT_EQUAL( m_progressUrl, url, () );
    m_url[m_downloadsProcessed] = url;
    m_result[m_downloadsProcessed] = successfully;
    ++m_downloadsProcessed;
    if (m_downloadsProcessed >= TMaxDownloadsNum)
      STOP_WAITING_FOR_ASYNC_DOWNLOAD;  // return control to test function body
  }

  void OnDownloadProgress(char const * url, TDownloadProgress progress)
  {
    m_progressUrl = url;
    if (progress.second < 0)
    {
      cerr << "Your hosting doesn't support total file size or invalid resume range was given for " << url << endl;
    }
    // for big file - cancel downloading after 40Kb were transferred
    if (progress.first > 40 * 1024)
      gMgr.CancelDownload(url);
  }
};

UNIT_TEST(SingleDownload)
{
  size_t const NUM = 1;
  DlObserver<NUM> observer;

  FileWriter::DeleteFile(TEST_FILE_NAME1);
  gMgr.DownloadFile(TEST_FILE_URL1, TEST_FILE_NAME1,
      boost::bind(&DlObserver<NUM>::OnDownloadFinished, &observer, _1, _2),
      boost::bind(&DlObserver<NUM>::OnDownloadProgress, &observer, _1, _2));
  WAIT_FOR_ASYNC_DOWNLOAD;
  TEST( observer.m_result[0], ("Do you have internet connection?") );
  TEST( gPlatform.IsFileExists(TEST_FILE_NAME1), () );
  FileWriter::DeleteFile(TEST_FILE_NAME1);
}

UNIT_TEST(MultiDownload)
{
  size_t const NUM = 3;
  DlObserver<NUM> observer;

  FileWriter::DeleteFile(TEST_FILE_NAME1);
  FileWriter::DeleteFile(TEST_FILE_NAME2);
  FileWriter::DeleteFile(TEST_FILE_NAME3);
  gMgr.DownloadFile(TEST_FILE_URL1, TEST_FILE_NAME1,
      boost::bind(&DlObserver<NUM>::OnDownloadFinished, &observer, _1, _2),
      boost::bind(&DlObserver<NUM>::OnDownloadProgress, &observer, _1, _2));
  gMgr.DownloadFile(TEST_FILE_URL2, TEST_FILE_NAME2,
      boost::bind(&DlObserver<NUM>::OnDownloadFinished, &observer, _1, _2),
      boost::bind(&DlObserver<NUM>::OnDownloadProgress, &observer, _1, _2));
  gMgr.DownloadFile(TEST_FILE_URL3, TEST_FILE_NAME3,
      boost::bind(&DlObserver<NUM>::OnDownloadFinished, &observer, _1, _2),
      boost::bind(&DlObserver<NUM>::OnDownloadProgress, &observer, _1, _2));
  WAIT_FOR_ASYNC_DOWNLOAD;

  TEST( observer.m_result[0], ("Do you have internet connection?") );
  TEST( gPlatform.IsFileExists(TEST_FILE_NAME1), () );
  FileWriter::DeleteFile(TEST_FILE_NAME1);

  TEST( observer.m_result[1], ("Do you have internet connection?") );
  TEST( gPlatform.IsFileExists(TEST_FILE_NAME2), () );
  FileWriter::DeleteFile(TEST_FILE_NAME2);

  TEST( observer.m_result[2], ("Do you have internet connection?") );
  TEST( gPlatform.IsFileExists(TEST_FILE_NAME3), () );
  FileWriter::DeleteFile(TEST_FILE_NAME3);
}

UNIT_TEST(InvalidUrl)
{
  size_t const NUM = 1;
  DlObserver<NUM> observer;
  // this should be set to false on error
  observer.m_result[0] = true;

  gMgr.DownloadFile(TEST_INVALID_URL, TEST_FILE_NAME1,
      boost::bind(&DlObserver<NUM>::OnDownloadFinished, &observer, _1, _2),
      boost::bind(&DlObserver<NUM>::OnDownloadProgress, &observer, _1, _2));
  WAIT_FOR_ASYNC_DOWNLOAD;

  TEST_EQUAL( observer.m_result[0], false, () );

  FileWriter::DeleteFile(TEST_FILE_NAME1);
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

  gMgr.DownloadFile(TEST_FILE_URL1, TEST_FILE_NAME1,
      boost::bind(&DlObserver<NUM>::OnDownloadFinished, &observer, _1, _2),
      boost::bind(&DlObserver<NUM>::OnDownloadProgress, &observer, _1, _2));
  WAIT_FOR_ASYNC_DOWNLOAD;

  TEST_EQUAL( observer.m_result[0], true, () );

  {
    string str;
    ReadFileToString(TEST_FILE_NAME1, str);
    TEST_NOT_EQUAL(fileContent, str, ("File should be overwritten by download manager"));
  }

  FileWriter::DeleteFile(TEST_FILE_NAME1);
}
/* not actual, now dlmgr doesn't notify client on canceling
UNIT_TEST(LongDownloadCanceling)
{
  // download is canceled inside OnDownloadProgress method in observer
  DlObserver<1> observer;
  observer.m_result[0] = true;
  gMgr.DownloadFile(TEST_BIG_FILE_URL, TEST_FILE_NAME1, observer);
  WAIT_FOR_ASYNC_DOWNLOAD;

  TEST_EQUAL( observer.m_result[0], false, () );
  TEST_EQUAL( gPlatform.IsFileExists(TEST_FILE_NAME1), false, ("File should be deleted if download was canceled") );

  FileWriter::DeleteFile(TEST_FILE_NAME1);
}
*/
UNIT_TEST(DownloadResume)
{
  size_t const NUM = 1;

  string const fileContent("aLeX");

  {
    FileWriter f(TEST_FILE_NAME1 DOWNLOADING_FILE_EXTENSION);
    f.Write(fileContent.c_str(), fileContent.size());
  }

 DlObserver<NUM> observer1;
  gMgr.DownloadFile(TEST_FILE_URL1, TEST_FILE_NAME1,
      boost::bind(&DlObserver<NUM>::OnDownloadFinished, &observer1, _1, _2),
      boost::bind(&DlObserver<NUM>::OnDownloadProgress, &observer1, _1, _2),
      false);
  WAIT_FOR_ASYNC_DOWNLOAD;
  TEST_EQUAL( observer1.m_result[0], true, () );

  DlObserver<NUM> observer2;
  gMgr.DownloadFile(TEST_FILE_URL1, TEST_FILE_NAME2,
      boost::bind(&DlObserver<NUM>::OnDownloadFinished, &observer2, _1, _2),
      boost::bind(&DlObserver<NUM>::OnDownloadProgress, &observer2, _1, _2),
      false);
  WAIT_FOR_ASYNC_DOWNLOAD;
  TEST_EQUAL( observer2.m_result[0], true, () );

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
  gMgr.DownloadFile(TEST_FILE_URL1, TEST_FILE_NAME1,
      boost::bind(&DlObserver<NUM>::OnDownloadFinished, &observer3, _1, _2),
      boost::bind(&DlObserver<NUM>::OnDownloadProgress, &observer3, _1, _2),
      true);
  WAIT_FOR_ASYNC_DOWNLOAD;
  TEST_EQUAL( observer3.m_result[0], true, () );

  TEST( GetPlatform().GetFileSize(TEST_FILE_NAME1, size1), ());
  TEST_EQUAL( size1, size2, () );

  {
    string str;
    ReadFileToString(TEST_FILE_NAME1, str);
    TEST_EQUAL(fileContent, str.substr(0, fileContent.size()), ());
    TEST_NOT_EQUAL(fileContent, str, ());
  }

  FileWriter::DeleteFile(TEST_FILE_NAME1);
  FileWriter::DeleteFile(TEST_FILE_NAME2);
}

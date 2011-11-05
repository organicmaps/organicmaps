#include "../../testing/testing.hpp"

#include "../../base/logging.hpp"

#include "../../coding/writer.hpp"

#include "../../platform/http_request.hpp"

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
    delete &request;
    QCoreApplication::quit();
  }
  void OnFinish(HttpRequest &)
  {
    TEST(false, ("Should be never called"));
  }
};

UNIT_TEST(DownloaderSimpleGet)
{
  DownloadObserver observer;
  string buffer;
  MemWriter<string> writer(buffer);
  HttpRequest::CallbackT onFinish = bind(&DownloadObserver::OnDownloadFinish, &observer, _1);
  HttpRequest::CallbackT onProgress = bind(&DownloadObserver::OnDownloadProgress, &observer, _1);
  { // simple success case
    scoped_ptr<HttpRequest> request(HttpRequest::Get(TEST_URL1, writer, onFinish, onProgress));
    // wait until download is finished
    QCoreApplication::exec();
    observer.TestOk();
    TEST_EQUAL(buffer, "Test1", (buffer));
  }

  buffer.clear();
  writer.Seek(0);
  observer.Reset();
  { // permanent redirect success case
    scoped_ptr<HttpRequest> request(HttpRequest::Get(TEST_URL_PERMANENT, writer, onFinish, onProgress));
    QCoreApplication::exec();
    observer.TestOk();
    TEST_EQUAL(buffer, "Test1", (buffer));
  }

  buffer.clear();
  writer.Seek(0);
  observer.Reset();
  { // fail case 404
    scoped_ptr<HttpRequest> request(HttpRequest::Get(TEST_URL_404, writer, onFinish, onProgress));
    QCoreApplication::exec();
    observer.TestFailed();
    TEST_EQUAL(buffer.size(), 0, (buffer));
  }

  buffer.clear();
  writer.Seek(0);
  observer.Reset();
  { // fail case not existing host
    scoped_ptr<HttpRequest> request(HttpRequest::Get(TEST_URL_INVALID_HOST, writer, onFinish, onProgress));
    QCoreApplication::exec();
    observer.TestFailed();
    TEST_EQUAL(buffer.size(), 0, (buffer));
  }

  buffer.clear();
  writer.Seek(0);
  {
    CancelDownload canceler;
    /// should be deleted in canceler
    HttpRequest::Get(TEST_URL_BIG_FILE, writer,
        bind(&CancelDownload::OnFinish, &canceler, _1),
        bind(&CancelDownload::OnProgress, &canceler, _1));
    QCoreApplication::exec();
    TEST_GREATER(buffer.size(), 0, ());
  }

  buffer.clear();
  writer.Seek(0);
  observer.Reset();
  { // https success case
    scoped_ptr<HttpRequest> request(HttpRequest::Get(TEST_URL_HTTPS, writer, onFinish, onProgress));
    // wait until download is finished
    QCoreApplication::exec();
    observer.TestOk();
    TEST_GREATER(buffer.size(), 0, (buffer));
  }
}

UNIT_TEST(DownloaderSimplePost)
{
  DownloadObserver observer;
  string buffer;
  MemWriter<string> writer(buffer);
  HttpRequest::CallbackT onFinish = bind(&DownloadObserver::OnDownloadFinish, &observer, _1);
  HttpRequest::CallbackT onProgress = bind(&DownloadObserver::OnDownloadProgress, &observer, _1);
  { // simple success case
    string const postData = "{\"jsonKey\":\"jsonValue\"}";
    scoped_ptr<HttpRequest> request(HttpRequest::Post(TEST_URL_POST, writer, postData, onFinish, onProgress));
    // wait until download is finished
    QCoreApplication::exec();
    observer.TestOk();
    TEST_EQUAL(buffer, postData, (buffer));
  }
}

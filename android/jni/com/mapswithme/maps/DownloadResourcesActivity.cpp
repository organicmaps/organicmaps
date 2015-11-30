#include "Framework.hpp"
#include "MapStorage.hpp"

#include "defines.hpp"

#include "coding/file_name_utils.hpp"
#include "coding/internal/file_data.hpp"
#include "coding/reader_streambuf.hpp"
#include "coding/url_encode.hpp"

#include "platform/platform.hpp"
#include "platform/http_request.hpp"
#include "platform/servers_list.hpp"

#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include "std/vector.hpp"
#include "std/string.hpp"
#include "std/bind.hpp"
#include "std/shared_ptr.hpp"


using namespace downloader;

/// Special error codes to notify GUI about free space
//@{
#define ERR_DOWNLOAD_SUCCESS         0
#define ERR_NOT_ENOUGH_MEMORY       -1
#define ERR_NOT_ENOUGH_FREE_SPACE   -2
#define ERR_STORAGE_DISCONNECTED    -3
#define ERR_DOWNLOAD_ERROR          -4
#define ERR_NO_MORE_FILES           -5
#define ERR_FILE_IN_PROGRESS        -6
//@}

struct FileToDownload
{
  vector<string> m_urls;
  string m_fileName;
  string m_pathOnSdcard;
  uint64_t m_fileSize;
};

//static string g_apkPath;
//static string g_sdcardPath;
static vector<FileToDownload> g_filesToDownload;
static int g_totalDownloadedBytes;
static int g_totalBytesToDownload;
static shared_ptr<HttpRequest> g_currentRequest;

extern "C"
{
  int HasSpaceForFiles(Platform & pl, string const & sdcardPath, size_t fileSize)
  {
    switch (pl.GetWritableStorageStatus(fileSize))
    {
      case Platform::STORAGE_DISCONNECTED: return ERR_STORAGE_DISCONNECTED;
      case Platform::NOT_ENOUGH_SPACE: return ERR_NOT_ENOUGH_FREE_SPACE;
      default: return fileSize;
    }
  }

  // Check if we need to download mandatory resource file.
  bool NeedToDownload(Platform & pl, string const & name, int size)
  {
    try
    {
      ModelReaderPtr reader(pl.GetReader(name));
      return false;
    }
    catch (RootException const &)
    {
      // TODO: A hack for a changed font file name. Remove when this stops being relevant.
      // This is because we don't want to force update users with the old font (which is good enough)
      if (name == "02_droidsans-fallback.ttf")
        return NeedToDownload(pl, "02_wqy-microhei.ttf", size); // Size is not checked anyway
    }
    return true;
  }

  JNIEXPORT jint JNICALL
  Java_com_mapswithme_maps_DownloadResourcesActivity_getBytesToDownload(JNIEnv * env, jobject thiz)
  {
    // clear all
    g_filesToDownload.clear();
    g_totalBytesToDownload = 0;
    g_totalDownloadedBytes = 0;

    Platform & pl = GetPlatform();
    string const path = pl.WritableDir();

    ReaderStreamBuf buffer(pl.GetReader("external_resources.txt"));
    istream in(&buffer);

    string name;
    int size;
    while (true)
    {
      in >> name;
      if (!in.good())
        break;

      in >> size;
      if (!in.good())
        break;

      if (NeedToDownload(pl, name, size))
      {
        LOG(LDEBUG, ("Should download", name, "sized", size, "bytes"));

        FileToDownload f;
        f.m_pathOnSdcard = path + name;
        f.m_fileName = name;
        f.m_fileSize = size;

        g_filesToDownload.push_back(f);
        g_totalBytesToDownload += size;
      }
    }

    int const res = HasSpaceForFiles(pl, path, g_totalBytesToDownload);
    if (res == ERR_STORAGE_DISCONNECTED)
      LOG(LWARNING, ("External file system is not available"));
    else if (res == ERR_NOT_ENOUGH_FREE_SPACE)
      LOG(LWARNING, ("Not enough space to extract files"));

    g_currentRequest.reset();

    return res;
  }

  void DownloadFileFinished(shared_ptr<jobject> obj, HttpRequest const & req)
  {
    HttpRequest::StatusT const status = req.Status();
    ASSERT_NOT_EQUAL(status, HttpRequest::EInProgress, ());

    int errorCode = 0;
    switch (status)
    {
    case HttpRequest::ECompleted:
      errorCode = ERR_DOWNLOAD_SUCCESS;
      break;
    case HttpRequest::EFailed:
      errorCode = ERR_DOWNLOAD_ERROR;
      break;
    };

    g_currentRequest.reset();

    if (errorCode == ERR_DOWNLOAD_SUCCESS)
    {
      FileToDownload & curFile = g_filesToDownload.back();

      LOG(LDEBUG, ("finished downloading", curFile.m_fileName, "sized", curFile.m_fileSize, "bytes"));

      g_totalDownloadedBytes += curFile.m_fileSize;

      LOG(LDEBUG, ("totalDownloadedBytes:", g_totalDownloadedBytes));

      g_filesToDownload.pop_back();
    }

    JNIEnv * env = jni::GetEnv();

    jmethodID methodID = jni::GetJavaMethodID(env, *obj.get(), "onDownloadFinished", "(I)V");
    env->CallVoidMethod(*obj.get(), methodID, errorCode);
  }

  void DownloadFileProgress(shared_ptr<jobject> obj, HttpRequest const & req)
  {
    //LOG(LDEBUG, (req.Progress().first, "bytes for", g_filesToDownload.back().m_fileName, "was downloaded"));

    FileToDownload & curFile = g_filesToDownload.back();

    jint curTotal = req.Progress().second;
    jint curProgress = req.Progress().first;
    jint glbTotal = g_totalBytesToDownload;
    jint glbProgress = g_totalDownloadedBytes + req.Progress().first;

    JNIEnv * env = jni::GetEnv();

    jmethodID methodID = jni::GetJavaMethodID(env, *obj.get(), "onDownloadProgress", "(IIII)V");
    env->CallVoidMethod(*obj.get(), methodID,
                         curTotal, curProgress,
                         glbTotal, glbProgress);
  }

  typedef HttpRequest::CallbackT CallbackT;

  void DownloadURLListFinished(HttpRequest const & req,
                               CallbackT const & onFinish, CallbackT const & onProgress)
  {
    FileToDownload & curFile = g_filesToDownload.back();

    LOG(LINFO, ("Finished URL list download for", curFile.m_fileName));

    GetServerListFromRequest(req, curFile.m_urls);

    storage::Storage const & storage = g_framework->Storage();
    for (size_t i = 0; i < curFile.m_urls.size(); ++i)
    {
      curFile.m_urls[i] = storage.GetFileDownloadUrl(curFile.m_urls[i], curFile.m_fileName);
      LOG(LDEBUG, (curFile.m_urls[i]));
    }

    g_currentRequest.reset(HttpRequest::GetFile(
        curFile.m_urls, curFile.m_pathOnSdcard, curFile.m_fileSize,
        onFinish, onProgress,
        512 * 1024, false));
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_DownloadResourcesActivity_cancelCurrentFile(JNIEnv * env, jobject thiz)
  {
    LOG(LDEBUG, ("cancelCurrentFile, currentRequest=", g_currentRequest.get()));
    g_currentRequest.reset();
  }

  JNIEXPORT int JNICALL
  Java_com_mapswithme_maps_DownloadResourcesActivity_startNextFileDownload(JNIEnv * env,
      jobject thiz, jobject observer)
  {
    if (g_filesToDownload.empty())
      return ERR_NO_MORE_FILES;

    FileToDownload & curFile = g_filesToDownload.back();

    LOG(LDEBUG, ("downloading", curFile.m_fileName, "sized", curFile.m_fileSize, "bytes"));

    CallbackT onFinish(bind(&DownloadFileFinished, jni::make_global_ref(observer), _1));
    CallbackT onProgress(bind(&DownloadFileProgress, jni::make_global_ref(observer), _1));

    g_currentRequest.reset(HttpRequest::PostJson(
        GetPlatform().ResourcesMetaServerUrl(), curFile.m_fileName,
        bind(&DownloadURLListFinished, _1, onFinish, onProgress)));

    return ERR_FILE_IN_PROGRESS;
  }
}

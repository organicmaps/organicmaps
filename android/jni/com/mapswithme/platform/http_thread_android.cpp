#include "http_thread_android.hpp"

#include "../../../../../platform/http_thread_callback.hpp"

#include "../../../../../std/string.hpp"
#include "../../../../../base/logging.hpp"

#include "../maps/DownloadUI.hpp"
#include "../jni/jni_thread.hpp"

// @TODO empty stub, add android implementation

HttpThread::HttpThread(string const & url,
                       downloader::IHttpThreadCallback & cb,
                       int64_t beg,
                       int64_t end,
                       int64_t size,
                       string const & pb)
{
  LOG(LINFO, ("creating httpThread: ", &cb, url, beg, end, size, pb));

  /// should create java object here.
  JNIEnv * env = jni::GetCurrentThreadJNIEnv();

  LOG(LINFO, ("env : ", env));

  jclass k = env->FindClass("com/mapswithme/maps/downloader/DownloadChunkTask");

  jni::Method ctor(k, "<init>", "(JLjava/lang/String;JJJLjava/lang/String;)V");

  jlong _id = reinterpret_cast<jlong>(&cb);
  jlong _beg = static_cast<jlong>(beg);
  jlong _end = static_cast<jlong>(end);
  jlong _size = static_cast<jlong>(size);
  jstring _url = env->NewStringUTF(url.c_str());
  jstring _pb = env->NewStringUTF(pb.c_str());

  m_self = env->NewObject(k, ctor.GetMethodID(), _id, _url, _beg, _end, _size, _pb);

  LOG(LINFO, ("starting a newly created thread", m_self));

  jni::Method startFn(k, "start", "()V");

  startFn.CallVoid(m_self);

  LOG(LINFO, ("started separate download thread"));
}

HttpThread::~HttpThread()
{
  LOG(LINFO, ("destroying http_thread"));
  JNIEnv * env = jni::GetCurrentThreadJNIEnv();

  jclass k = env->FindClass("com/mapswithme/maps/downloader/DownloadChunkTask");
  jni::Method cancelFn(k, "cancel", "(Z)V");
  cancelFn.CallVoid(m_self, true);

  env->DeleteLocalRef(m_self);
}

namespace downloader
{

  HttpThread * CreateNativeHttpThread(string const & url,
                                      downloader::IHttpThreadCallback & cb,
                                      int64_t beg,
                                      int64_t end,
                                      int64_t size,
                                      string const & pb)
  {
    return new HttpThread(url, cb, beg, end, size, pb);
  }

  void DeleteNativeHttpThread(HttpThread * request)
  {
    delete request;
  }

} // namespace downloader

extern "C"
{
  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_downloader_DownloadChunkTask_onWrite(JNIEnv * env, jobject thiz,
      jlong httpCallbackID, jlong beg, jcharArray data, jlong size)
  {
    LOG(LINFO, ("onWrite: ", beg, size));
    downloader::IHttpThreadCallback * cb = reinterpret_cast<downloader::IHttpThreadCallback*>(httpCallbackID);
    JNIEnv * env0 = jni::GetCurrentThreadJNIEnv();
    jchar * buf = env0->GetCharArrayElements(data, 0);
    cb->OnWrite(beg, buf, size);
    env0->ReleaseCharArrayElements(data, buf, 0);
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_downloader_DownloadChunkTask_onFinish(JNIEnv * env, jobject thiz,
      jlong httpCallbackID, jlong httpCode, jlong beg, jlong end)
  {
    LOG(LINFO, ("onFinish: ", httpCode, beg, end));
    downloader::IHttpThreadCallback * cb = reinterpret_cast<downloader::IHttpThreadCallback*>(httpCallbackID);
    cb->OnFinish(httpCode, beg, end);
  }
}

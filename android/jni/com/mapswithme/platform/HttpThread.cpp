#include "../../../../../platform/http_thread_callback.hpp"

#include "../maps/DownloadUI.hpp"

class HttpThread
{
private:
  jobject m_self;

public:
  HttpThread(string const & url,
             downloader::IHttpThreadCallback & cb,
             int64_t beg,
             int64_t end,
             int64_t expectedFileSize,
             string const & pb)
  {
    /// should create java object here.
    JNIEnv * env = jni::GetCurrentThreadJNIEnv();

    jclass klass = env->FindClass("com/mapswithme/maps/downloader/DownloadChunkTask");
    ASSERT(klass, ("Can't find java class com/mapswithme/maps/downloader/DownloadChunkTask"));

    jmethodID methodId = env->GetMethodID(klass, "<init>", "(JLjava/lang/String;JJJLjava/lang/String;)V");
    ASSERT(methodId, ("Can't find java constructor in com/mapswithme/maps/downloader/DownloadChunkTask"));

    m_self = env->NewObject(klass, methodId, reinterpret_cast<jlong>(&cb),
        env->NewStringUTF(url.c_str()), beg, end, expectedFileSize, env->NewStringUTF(pb.c_str()));

    methodId = env->GetMethodID(klass, "start", "()V");
    ASSERT(methodId, ("Can't find java method 'start' in com/mapswithme/maps/downloader/DownloadChunkTask"));

    env->CallVoidMethod(m_self, methodId);
  }

  ~HttpThread()
  {
    JNIEnv * env = jni::GetCurrentThreadJNIEnv();
    jclass klass = env->FindClass("com/mapswithme/maps/downloader/DownloadChunkTask");
    ASSERT(klass, ("Can't find java class com/mapswithme/maps/downloader/DownloadChunkTask"));

    jmethodID methodId = env->GetMethodID(klass, "cancel", "(Z)Z");
    ASSERT(methodId, ("Can't find java method 'cancel' in com/mapswithme/maps/downloader/DownloadChunkTask"));

    env->CallBooleanMethod(m_self, methodId, false);
  }
};

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
      jlong httpCallbackID, jlong beg, jbyteArray data, jlong size)
  {
    downloader::IHttpThreadCallback * cb = reinterpret_cast<downloader::IHttpThreadCallback*>(httpCallbackID);
    JNIEnv * env0 = jni::GetCurrentThreadJNIEnv();
    jbyte * buf = env0->GetByteArrayElements(data, 0);
    cb->OnWrite(beg, buf, size);
    env0->ReleaseByteArrayElements(data, buf, 0);
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_downloader_DownloadChunkTask_onFinish(JNIEnv * env, jobject thiz,
      jlong httpCallbackID, jlong httpCode, jlong beg, jlong end)
  {
    downloader::IHttpThreadCallback * cb = reinterpret_cast<downloader::IHttpThreadCallback*>(httpCallbackID);
    cb->OnFinish(httpCode, beg, end);
  }
}

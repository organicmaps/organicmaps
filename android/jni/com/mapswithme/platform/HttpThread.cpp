#include "platform/http_thread_callback.hpp"

#include "../core/jni_helper.hpp"

#include "Platform.hpp"

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
    JNIEnv * env = jni::GetEnv();
    ASSERT ( env, () );

    jclass klass = env->FindClass("com/mapswithme/maps/downloader/DownloadChunkTask");
    ASSERT ( klass, () );

    static jmethodID initMethodId = env->GetMethodID(klass, "<init>", "(JLjava/lang/String;JJJ[BLjava/lang/String;)V");
    ASSERT ( initMethodId, () );

    // User id is always the same, so do not waste time on every chunk call
    static string uniqueUserId = GetPlatform().UniqueClientId();

    jbyteArray postBody = 0;
    size_t const postBodySize = pb.size();
    if (postBodySize)
    {
      postBody = env->NewByteArray(postBodySize);
      env->SetByteArrayRegion(postBody, 0, postBodySize, reinterpret_cast<jbyte const *>(pb.c_str()));
    }

    jstring jUrl = env->NewStringUTF(url.c_str());
    jstring jUserId = env->NewStringUTF(uniqueUserId.c_str());
    jobject const localSelf = env->NewObject(klass,
                                             initMethodId,
                                             reinterpret_cast<jlong>(&cb),
                                             jUrl,
                                             static_cast<jlong>(beg),
                                             static_cast<jlong>(end),
                                             static_cast<jlong>(expectedFileSize),
                                             postBody,
                                             jUserId);
    m_self = env->NewGlobalRef(localSelf);
    ASSERT ( m_self, () );

    env->DeleteLocalRef(localSelf);
    env->DeleteLocalRef(postBody);
    env->DeleteLocalRef(jUrl);
    env->DeleteLocalRef(jUserId);

    static jmethodID startMethodId = env->GetMethodID(klass, "start", "()V");
    ASSERT ( startMethodId, () );

    env->DeleteLocalRef(klass);

    env->CallVoidMethod(m_self, startMethodId);
  }

  ~HttpThread()
  {
    JNIEnv * env = jni::GetEnv();
    ASSERT ( env, () );

    jmethodID methodId = jni::GetJavaMethodID(env, m_self, "cancel", "(Z)Z");
    ASSERT ( methodId, () );

    env->CallBooleanMethod(m_self, methodId, false);

    env->DeleteGlobalRef(m_self);
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
  JNIEXPORT jboolean JNICALL
  Java_com_mapswithme_maps_downloader_DownloadChunkTask_onWrite(JNIEnv * env, jobject thiz,
      jlong httpCallbackID, jlong beg, jbyteArray data, jlong size)
  {
    downloader::IHttpThreadCallback * cb = reinterpret_cast<downloader::IHttpThreadCallback*>(httpCallbackID);
    jbyte * buf = env->GetByteArrayElements(data, 0);
    ASSERT ( buf, () );

    bool const ret = cb->OnWrite(beg, buf, size);
    env->ReleaseByteArrayElements(data, buf, 0);
    return ret;
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_downloader_DownloadChunkTask_onFinish(JNIEnv * env, jobject thiz,
      jlong httpCallbackID, jlong httpCode, jlong beg, jlong end)
  {
    downloader::IHttpThreadCallback * cb = reinterpret_cast<downloader::IHttpThreadCallback*>(httpCallbackID);
    cb->OnFinish(httpCode, beg, end);
  }
}

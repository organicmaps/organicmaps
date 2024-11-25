#include "AndroidPlatform.hpp"
#include "app/organicmaps/core/jni_helper.hpp"

#include "base/logging.hpp"
#include "network/http/thread.hpp"
#include "network/http/thread_callback.hpp"

namespace om::network::http
{
class Thread
{
private:
  jobject m_self;
  jclass m_klass;

public:
  Thread(std::string const & url, om::network::http::IThreadCallback & cb, int64_t beg, int64_t end,
         int64_t expectedFileSize, std::string const & pb)
  {
    JNIEnv * env = jni::GetEnv();
    static jclass const klass = jni::GetGlobalClassRef(env, "app/organicmaps/downloader/ChunkTask");
    m_klass = klass;
    // public ChunkTask(long httpCallbackID, String url, long beg, long end,
    //                  long expectedFileSize, byte[] postBody, String userAgent)
    static jmethodID const initMethodId = jni::GetConstructorID(env, klass, "(JLjava/lang/String;JJJ[B)V");
    static jmethodID const startMethodId = env->GetMethodID(klass, "start", "()V");

    jni::TScopedLocalByteArrayRef postBody(env, nullptr);
    jsize const postBodySize = static_cast<jsize>(pb.size());
    if (postBodySize)
    {
      postBody.reset(env->NewByteArray(postBodySize));
      env->SetByteArrayRegion(postBody.get(), 0, postBodySize, reinterpret_cast<jbyte const *>(pb.c_str()));
    }

    jni::TScopedLocalRef jUrl(env, jni::ToJavaString(env, url.c_str()));
    jni::TScopedLocalRef localSelf(
        env, env->NewObject(klass, initMethodId, reinterpret_cast<jlong>(&cb), jUrl.get(), static_cast<jlong>(beg),
                            static_cast<jlong>(end), static_cast<jlong>(expectedFileSize), postBody.get()));
    m_self = env->NewGlobalRef(localSelf.get());
    ASSERT(m_self, ());

    env->CallVoidMethod(m_self, startMethodId);
  }

  ~Thread()
  {
    JNIEnv * env = jni::GetEnv();
    static jmethodID const cancelMethodId = env->GetMethodID(m_klass, "cancel", "(Z)Z");
    env->CallBooleanMethod(m_self, cancelMethodId, false);
    env->DeleteGlobalRef(m_self);
  }
};

namespace thread
{
Thread * CreateThread(std::string const & url, IThreadCallback & cb, int64_t beg, int64_t end, int64_t size,
                                std::string const & pb)
{
  return new Thread(url, cb, beg, end, size, pb);
}

void DeleteThread(Thread * request) { delete request; }

}  // namespace thread
}  // namespace om::network::http

extern "C"
{
JNIEXPORT jboolean JNICALL
Java_app_organicmaps_downloader_ChunkTask_nativeOnWrite(JNIEnv * env, jclass clazz, jlong httpCallbackID, jlong beg, jbyteArray data, jlong size)
{
  om::network::http::IThreadCallback * cb = reinterpret_cast<om::network::http::IThreadCallback*>(httpCallbackID);
  jbyte * buf = env->GetByteArrayElements(data, 0);
  ASSERT(buf, ());

  bool ret = false;
  try
  {
    ret = cb->OnWrite(beg, buf, static_cast<jsize>(size));
  }
  catch (std::exception const & ex)
  {
    LOG(LERROR, ("Failed to write chunk:", ex.what()));
  }

  env->ReleaseByteArrayElements(data, buf, 0);
  return ret;
}

JNIEXPORT void JNICALL
Java_app_organicmaps_downloader_ChunkTask_nativeOnFinish(JNIEnv * env, jclass clazz, jlong httpCallbackID, jlong httpCode, jlong beg, jlong end)
{
  om::network::http::IThreadCallback * cb = reinterpret_cast<om::network::http::IThreadCallback*>(httpCallbackID);
  cb->OnFinish(static_cast<long>(httpCode), beg, end);
}
} // extern "C"

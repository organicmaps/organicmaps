#include "../core/jni_helper.hpp"
#include "platform/socket.hpp"
#include "base/logging.hpp"

namespace platform
{
class SocketImpl : public Socket
{
public:
  SocketImpl()
  {
    JNIEnv * env = jni::GetEnv();
    static jmethodID const socketConstructor = jni::GetConstructorID(env, g_socketWrapperClazz, "()V");
    jni::TScopedLocalRef localSelf(env, env->NewObject(g_socketWrapperClazz, socketConstructor));
    m_self = env->NewGlobalRef(localSelf.get());
    ASSERT(m_self, ());
  }

  ~SocketImpl()
  {
    Close();
    JNIEnv * env = jni::GetEnv();
    env->DeleteGlobalRef(m_self);
  }

  bool Open(string const & host, uint16_t port)
  {
    JNIEnv * env = jni::GetEnv();
    static jmethodID const openMethod =
        jni::GetMethodID(env, m_self, "open", "(Ljava/lang/String;I)Z");
    return env->CallBooleanMethod(m_self, openMethod, jni::ToJavaString(env, host),
                                  static_cast<jint>(port));
  }

  void Close()
  {
    JNIEnv * env = jni::GetEnv();
    static jmethodID const closeMethod = jni::GetMethodID(env, m_self, "close", "()V");
    env->CallVoidMethod(m_self, closeMethod);
  }

  bool Read(uint8_t * data, uint32_t count)
  {
    JNIEnv * env = jni::GetEnv();
    jbyteArray array = env->NewByteArray(count);
    static jmethodID const readMethod = jni::GetMethodID(env, m_self, "read", "([BI)Z");
    jboolean result = env->CallBooleanMethod(m_self, readMethod, array, static_cast<jint>(count));
    //this call copies java byte array to native buffer
    env->GetByteArrayRegion(array, 0, count, reinterpret_cast<jbyte *>(data));
    return result;
  }

  bool Write(uint8_t const * data, uint32_t count)
  {
    JNIEnv * env = jni::GetEnv();
    jbyteArray array = env->NewByteArray(count);
    //this call copies native buffer to java byte array
    env->SetByteArrayRegion(array, 0, count, reinterpret_cast<const jbyte *>(data));
    static jmethodID const writeMethod = jni::GetMethodID(env, m_self, "write", "([BI)Z");
    return env->CallBooleanMethod(m_self, writeMethod, array, static_cast<jint>(count));
  }

  void SetTimeout(uint32_t milliseconds)
  {
    JNIEnv * env = jni::GetEnv();
    static jmethodID const setTimeoutMethod = jni::GetMethodID(env, m_self, "setTimeout", "(I)V");
    env->CallVoidMethod(m_self, setTimeoutMethod, static_cast<jint>(milliseconds));
  };

private:
  jobject m_self;
};

unique_ptr<Socket> CreateSocket() { return make_unique<SocketImpl>(); }
}

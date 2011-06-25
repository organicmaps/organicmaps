#include "jni_string.h"

namespace jni {

  String::String(JNIEnv * env, jstring s)
  : m_env(env), m_s(s)
  {
    m_ret = m_env->GetStringChars(m_s, NULL);
  }

  String::~String()
  {
    m_env->ReleaseStringChars(m_s, m_ret);
  }

  string String::ToString() const
  {
    /// @todo
    return std::string();
  }

  string ToString(JNIEnv * env, jstring s)
  {
    return String(env, s).ToString();
  }
}

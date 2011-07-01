#include "jni_string.h"

#include "../../base/string_utils.hpp"


namespace jni
{
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
    size_t const sz = m_env->GetStringLength(m_s);

    string res;
    utf8::unchecked::utf16to8(m_ret, m_ret + sz, back_inserter(res));

    return res;
  }

  string ToString(JNIEnv * env, jstring s)
  {
    return String(env, s).ToString();
  }
}

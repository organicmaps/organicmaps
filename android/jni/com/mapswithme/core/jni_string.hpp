#pragma once

#include <string.h>
#include <jni.h>

#include "../../../../../std/string.hpp"

namespace jni
{
  class String
  {
    JNIEnv * m_env;
    jstring m_s;
    jchar const * m_ret;
  public:
    String(JNIEnv * env, jstring s);
    ~String();

    string ToString() const;
  };

  string ToString(JNIEnv * env, jstring s);
}

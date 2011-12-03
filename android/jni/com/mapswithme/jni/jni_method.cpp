/*
 * method_ref.cpp
 *
 *  Created on: Nov 27, 2011
 *      Author: siarheirachytski
 */

#include "jni_thread.hpp"
#include "jni_method.hpp"
#include "../../../../../base/assert.hpp"

namespace jni
{
  Method::Method(jclass klass,
                 char const * name,
                 char const * signature)
               : m_name(name),
                 m_signature(signature)
  {
    JNIEnv * env = GetCurrentThreadJNIEnv();
    m_index = env->GetMethodID(klass, m_name, m_signature);
    CHECK(m_index, ("Error: No valid function pointer in ", m_name));
  }

  Method::Method(jobject obj,
                 char const * name,
                 char const * signature)
               : m_name(name),
                 m_signature(signature)
  {
    jclass k = GetCurrentThreadJNIEnv()->GetObjectClass(obj);
    GetCurrentThreadJNIEnv()->GetMethodID(k, m_name, m_signature);
    CHECK(m_index, ("Error: No valid function pointer in ", m_name));
  }

  bool Method::CallBoolean(jobject self)
  {
    JNIEnv* jniEnv = GetCurrentThreadJNIEnv();

    CHECK(jniEnv, ("Error: No valid JNI env in ", m_name));

    return jniEnv->CallBooleanMethod(self, m_index);
  }

  bool Method::CallInt(jobject self)
  {
    JNIEnv* jniEnv = GetCurrentThreadJNIEnv();

    CHECK(jniEnv, ("Error: No valid JNI env in ", m_name));

    return (int)jniEnv->CallIntMethod(self, m_index);
  }

  jmethodID Method::GetMethodID() const
  {
    return m_index;
  }
}

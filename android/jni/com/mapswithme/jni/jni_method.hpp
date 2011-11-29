/*
 * method_ref.hpp
 *
 *  Created on: Nov 27, 2011
 *      Author: siarheirachytski
 */

#pragma once

#include <jni.h>
#include "jni_thread.hpp"

namespace jni
{
  class Method
  {
  private:

    const char* m_name;
    const char* m_signature;
    jmethodID m_index;

  public:

    Method(jclass klass,
           const char* name,
           const char* signature);

    void CallVoid(jobject self)
    {
      GetCurrentThreadJNIEnv()->CallVoidMethod(self, m_index);
    };

    template <typename A1>
    void CallVoid(jobject self, A1 a1)
    {
      GetCurrentThreadJNIEnv()->CallVoidMethod(self, m_index, a1);
    }

    template <typename A1, typename A2>
    void CallVoid(jobject self, A1 a1, A2 a2)
    {
      GetCurrentThreadJNIEnv()->CallVoidMethod(self, m_index, a1, a2);
    }

    template <typename A1, typename A2, typename A3>
    void CallVoid(jobject self, A1 a1, A2 a2, A3 a3)
    {
      GetCurrentThreadJNIEnv()->CallVoidMethod(self, m_index, a1, a2, a3);
    }

    template <typename A1, typename A2, typename A3, typename A4>
    void CallVoid(jobject self, A1 a1, A2 a2, A3 a3, A4 a4)
    {
      GetCurrentThreadJNIEnv()->CallVoidMethod(self, m_index, a1, a2, a3, a4);
    }

    template <typename A1, typename A2, typename A3, typename A4, typename A5>
    void CallVoid(jobject self, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5)
    {
      GetCurrentThreadJNIEnv()->CallVoidMethod(self, m_index, a1, a2, a3, a4, a5);
    }

    bool CallBoolean(jobject self);
    bool CallInt(jobject self);
  };
}

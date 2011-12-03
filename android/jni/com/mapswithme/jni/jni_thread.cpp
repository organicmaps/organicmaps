/*
 * jni_thread.cpp
 *
 *  Created on: Nov 27, 2011
 *      Author: siarheirachytski
 */

#include "jni_thread.hpp"
#include <pthread.h>
#include "../../../../../base/logging.hpp"

namespace jni
{
  JavaVM * s_jvm;

  void SetCurrentJVM(JavaVM * jvm)
  {
    s_jvm = jvm;
  }

  JavaVM * GetCurrentJVM()
  {
    if (s_jvm == 0)
      LOG(LINFO, ("no current JVM"));
    return s_jvm;
  }

  static pthread_key_t s_jniEnvKey = 0;

  JNIEnv* GetCurrentThreadJNIEnv()
  {
    JNIEnv* env = NULL;
    if (s_jniEnvKey)
      env = (JNIEnv*)pthread_getspecific(s_jniEnvKey);
    else
      pthread_key_create(&s_jniEnvKey, NULL);

    if (!env)
    {
      if (!GetCurrentJVM())
      {
        LOG(LINFO, ("Error - could not find JVM!"));
        return 0;
      }

      // Hmm - no env for this thread cached yet
      int error = GetCurrentJVM()->AttachCurrentThread(&env, 0);

      LOG(LINFO, ("AttachCurrentThread: ", error, ", ", env));
      if (error || !env)
      {
        LOG(LINFO, ("Error - could not attach thread to JVM!"));
        return 0;
      }

      pthread_setspecific(s_jniEnvKey, env);
    }

    return env;
  }

}




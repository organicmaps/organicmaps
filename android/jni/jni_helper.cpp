#include "jni_helper.h"

#include <assert.h>


namespace jni {

  // Some examples of sig:
  // "()V" - void function returning void;
  // "(Ljava/lang/String;)V" - String function returning void;
  jmethodID GetJavaMethodID(JNIEnv * env, jobject obj,
                            char const * fn, char const * sig)
  {
    jclass cls = env->GetObjectClass(obj);
    jmethodID mid = env->GetMethodID(cls, fn, sig);
    assert(mid != 0);
    return mid;
  }

}

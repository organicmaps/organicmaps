#include "jni_helper.hpp"

#include "../../../../../base/assert.hpp"


namespace jni {

  // Some examples of sig:
  // "()V" - void function returning void;
  // "(Ljava/lang/String;)V" - String function returning void;
  jmethodID GetJavaMethodID(JNIEnv * env, jobject obj,
                            char const * fn, char const * sig)
  {
    ASSERT ( env != 0 && obj != 0, () );
    jclass cls = env->GetObjectClass(obj);
    jmethodID mid = env->GetMethodID(cls, fn, sig);
    ASSERT ( mid != 0, () );
    return mid;
  }

}

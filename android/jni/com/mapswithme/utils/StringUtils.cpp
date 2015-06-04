#include "base/string_utils.hpp"
#include "../core/jni_helper.hpp"

extern "C"
{
  JNIEXPORT jboolean JNICALL
  Java_com_mapswithme_util_StringUtils_isHtml(JNIEnv * env, jclass thiz, jstring text)
  {
    return strings::IsHTML(jni::ToNativeString(env, text));
  }

} // extern "C"

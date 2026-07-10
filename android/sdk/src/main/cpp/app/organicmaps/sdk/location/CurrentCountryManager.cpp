#include "app/organicmaps/sdk/Framework.hpp"

#include "app/organicmaps/sdk/core/jni_helper.hpp"
#include "app/organicmaps/sdk/core/jni_java_methods.hpp"

extern "C"
{
// static void nativeSetListener(@NonNull OnCountryChangedListener listener);
JNIEXPORT void Java_app_organicmaps_sdk_location_CurrentCountryManager_nativeSetListener(JNIEnv *, jclass,
                                                                                         jobject listener)
{
  frm()->SetLocationCountryChangedListener([listener = make_global_ref(listener)](storage::CountryId const & countryId)
  {
    JNIEnv * env = jni::GetEnv();
    jmethodID methodID = jni::GetMethodID(env, *listener, "onCountryChanged", "(Ljava/lang/String;)V");
    env->CallVoidMethod(*listener, methodID, jni::TScopedLocalRef(env, jni::ToJavaString(env, countryId)).get());
  });
}

// static void nativeRemoveListener();
JNIEXPORT void Java_app_organicmaps_sdk_location_CurrentCountryManager_nativeRemoveListener(JNIEnv *, jclass)
{
  frm()->SetLocationCountryChangedListener(nullptr);
}
}

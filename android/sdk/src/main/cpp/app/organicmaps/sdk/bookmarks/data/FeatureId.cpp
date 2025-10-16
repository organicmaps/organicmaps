#include "FeatureId.hpp"

#include "app/organicmaps/sdk/core/jni_helper.hpp"

jobject CreateFeatureId(JNIEnv * env, FeatureID const & fid)
{
  static jmethodID const featureCtorId = jni::GetConstructorID(env, g_featureIdClazz, "(Ljava/lang/String;JI)V");

  auto const & info = fid.m_mwmId.GetInfo();
  jni::TScopedLocalRef jMwmName(env, jni::ToJavaString(env, info ? info->GetCountryName() : ""));
  return env->NewObject(g_featureIdClazz, featureCtorId, jMwmName.get(),
                        info ? static_cast<jlong>(info->GetVersion()) : 0, static_cast<jint>(fid.m_index));
}

jobjectArray ToFeatureIdArray(JNIEnv * env, std::vector<FeatureID> const & ids)
{
  if (ids.empty())
    return nullptr;

  return jni::ToJavaArray(env, g_featureIdClazz, ids,
                          [](JNIEnv * env, FeatureID const & fid) { return CreateFeatureId(env, fid); });
}

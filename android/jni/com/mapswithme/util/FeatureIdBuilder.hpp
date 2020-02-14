#pragma once

#include "com/mapswithme/core/jni_helper.hpp"

#include "indexer/feature_decl.hpp"

class FeatureIdBuilder
{
public:
  FeatureIdBuilder(JNIEnv * env)
  {
    m_class = jni::GetGlobalClassRef(env, "com/mapswithme/maps/bookmarks/data/FeatureId");
    m_countryName = env->GetFieldID(m_class, "mMwmName", "Ljava/lang/String;");
    m_version = env->GetFieldID(m_class, "mMwmVersion", "J");
    m_index = env->GetFieldID(m_class, "mFeatureIndex", "I");
  }

  FeatureID Build(JNIEnv * env, jobject obj) const
  {
    jstring jcountryName = static_cast<jstring>(env->GetObjectField(obj, m_countryName));
    jint jindex = env->GetIntField(obj, m_index);

    auto const countryName = jni::ToNativeString(env, jcountryName);
    auto const index = static_cast<uint32_t>(jindex);

    auto const & ds = g_framework->GetDataSource();
    auto const id = ds.GetMwmIdByCountryFile(platform::CountryFile(countryName));
    return FeatureID(id, index);
  }

private:
  jclass m_class;
  jfieldID m_countryName;
  jfieldID m_version;
  jfieldID m_index;
};

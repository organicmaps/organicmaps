///////////////////////////////////////////////////////////////////////////////////
// DownloadUI
///////////////////////////////////////////////////////////////////////////////////

#include "Framework.hpp"
#include "../core/jni_helper.hpp"

extern "C"
{
  class IndexBinding
  {
  private:

    shared_ptr<jobject> m_self;

    jfieldID m_groupID;
    jfieldID m_countryID;
    jfieldID m_regionID;

  public:

    IndexBinding(jobject self) : m_self(jni::make_global_ref(self))
    {
      jclass cls = jni::GetEnv()->GetObjectClass(*m_self.get());

      m_groupID = jni::GetEnv()->GetFieldID(cls, "mGroup", "I");
      m_countryID = jni::GetEnv()->GetFieldID(cls, "mCountry", "I");
      m_regionID = jni::GetEnv()->GetFieldID(cls, "mRegion", "I");
    }

    int group() const
    {
      return jni::GetEnv()->GetIntField(*m_self.get(), m_groupID);
    }

    int country() const
    {
      return jni::GetEnv()->GetIntField(*m_self.get(), m_countryID);
    }

    int region() const
    {
      return jni::GetEnv()->GetIntField(*m_self.get(), m_regionID);
    }

    storage::TIndex const toNative() const
    {
      return storage::TIndex(group(), country(), region());
    }

    static jobject toJava(storage::TIndex const & idx)
    {
      LOG(LDEBUG, ("constructing Java Index object from ", idx.m_group, idx.m_country, idx.m_region));

      JNIEnv * env = jni::GetEnv();

      jclass klass = env->FindClass("com/mapswithme/maps/MapStorage$Index");
      ASSERT(klass, ("Can't find java class com/mapswithme/maps/MapStorage$Index"));

      jmethodID methodId = env->GetMethodID(klass, "<init>", "(III)V");
      ASSERT(methodId, ("Can't find java constructor in com/mapswithme/maps/MapStorage$Index"));

      return env->NewObject(klass, methodId, idx.m_group, idx.m_country, idx.m_region);
    }
  };

  JNIEXPORT jint JNICALL
  Java_com_mapswithme_maps_MapStorage_countriesCount(JNIEnv * env, jobject thiz, jobject idx)
  {
    return static_cast<jint>(g_framework->Storage().CountriesCount(IndexBinding(idx).toNative()));
  }

  JNIEXPORT jstring JNICALL
  Java_com_mapswithme_maps_MapStorage_countryName(JNIEnv * env, jobject thiz, jobject idx)
  {
    string const name = g_framework->Storage().CountryName(IndexBinding(idx).toNative());
    return env->NewStringUTF(name.c_str());
  }

  JNIEXPORT jlong JNICALL
  Java_com_mapswithme_maps_MapStorage_countryLocalSizeInBytes(JNIEnv * env, jobject thiz, jobject idx)
  {
    return g_framework->Storage().CountrySizeInBytes(IndexBinding(idx).toNative()).first;
  }

  JNIEXPORT jlong JNICALL
  Java_com_mapswithme_maps_MapStorage_countryRemoteSizeInBytes(JNIEnv * env, jobject thiz, jobject idx)
  {
    return g_framework->Storage().CountrySizeInBytes(IndexBinding(idx).toNative()).second;
  }

  JNIEXPORT jint JNICALL
  Java_com_mapswithme_maps_MapStorage_countryStatus(JNIEnv * env, jobject thiz, jobject idx)
  {
    return static_cast<jint>(g_framework->Storage().CountryStatus(IndexBinding(idx).toNative()));
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_MapStorage_downloadCountry(JNIEnv * env, jobject thiz, jobject idx)
  {
    g_framework->Storage().DownloadCountry(IndexBinding(idx).toNative());
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_MapStorage_deleteCountry(JNIEnv * env, jobject thiz, jobject idx)
  {
    g_framework->Storage().DeleteCountry(IndexBinding(idx).toNative());
  }

  JNIEXPORT jobject JNICALL
  Java_com_mapswithme_maps_MapStorage_findIndexByName(JNIEnv * env, jobject thiz, jstring name)
  {
    char const * strCountry = env->GetStringUTFChars(name, 0);

    if (!strCountry)
      return IndexBinding::toJava(storage::TIndex());

    return IndexBinding::toJava(g_framework->Storage().FindIndexByName(strCountry));
  }

  void ReportChangeCountryStatus(shared_ptr<jobject> const & obj, storage::TIndex const & idx)
  {
    JNIEnv * env = jni::GetEnv();

    jclass klass = env->GetObjectClass(*obj.get());

    jmethodID methodID = env->GetMethodID(klass, "onCountryStatusChanged", "(Lcom/mapswithme/maps/MapStorage$Index;)V");

    env->CallVoidMethod(*obj.get(), methodID, IndexBinding::toJava(idx));
  }

  void ReportCountryProgress(shared_ptr<jobject> const & obj, storage::TIndex const & idx, pair<int64_t, int64_t> const & p)
  {
    JNIEnv * env = jni::GetEnv();
    jclass klass = env->GetObjectClass(*obj.get());

    jmethodID methodID = env->GetMethodID(klass, "onCountryProgress", "(Lcom/mapswithme/maps/MapStorage$Index;JJ)V");

    jlong current = p.first;
    jlong total = p.second;

    env->CallVoidMethod(*obj.get(), methodID, IndexBinding::toJava(idx), current, total);
  }

  JNIEXPORT jint JNICALL
  Java_com_mapswithme_maps_MapStorage_subscribe(JNIEnv * env, jobject thiz, jobject obs)
  {
    jint res = g_framework->Storage().Subscribe(bind(&ReportChangeCountryStatus, jni::make_global_ref(obs), _1),
                                                bind(&ReportCountryProgress, jni::make_global_ref(obs), _1, _2));
    return res;
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_MapStorage_unsubscribe(JNIEnv * env, jobject thiz, jint slotID)
  {
    g_framework->Storage().Unsubscribe(slotID);
  }
}

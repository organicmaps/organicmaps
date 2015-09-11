#include "MapStorage.hpp"
#include "Framework.hpp"
#include "com/mapswithme/country/country_helper.hpp"
#include "com/mapswithme/platform/Platform.hpp"

#include "coding/internal/file_data.hpp"


using namespace storage;

namespace
{
  ::Framework * frm()
  {
    return g_framework->NativeFramework();
  }

  Storage & GetStorage()
  {
    return frm()->Storage();
  }
}

extern "C"
{
  class IndexBinding
  {
  private:
    shared_ptr<jobject> m_self;

    jfieldID m_groupID;
    jfieldID m_countryID;
    jfieldID m_regionID;

    jobject object() const { return *m_self.get(); }

  public:
    IndexBinding(jobject self) : m_self(jni::make_global_ref(self))
    {
      jclass klass = jni::GetEnv()->GetObjectClass(object());

      m_groupID = jni::GetEnv()->GetFieldID(klass, "mGroup", "I");
      m_countryID = jni::GetEnv()->GetFieldID(klass, "mCountry", "I");
      m_regionID = jni::GetEnv()->GetFieldID(klass, "mRegion", "I");
    }

    int group() const
    {
      return jni::GetEnv()->GetIntField(object(), m_groupID);
    }

    int country() const
    {
      return jni::GetEnv()->GetIntField(object(), m_countryID);
    }

    int region() const
    {
      return jni::GetEnv()->GetIntField(object(), m_regionID);
    }

    TIndex const toNative() const
    {
      return TIndex(group(), country(), region());
    }
  };

  JNIEXPORT jstring JNICALL
  Java_com_mapswithme_maps_MapStorage_countryName(JNIEnv * env, jobject thiz, jobject idx)
  {
    string const name = GetStorage().CountryName(IndexBinding(idx).toNative());
    return env->NewStringUTF(name.c_str());
  }

  JNIEXPORT jlong JNICALL
  Java_com_mapswithme_maps_MapStorage_countryRemoteSizeInBytes(JNIEnv * env, jobject thiz, jobject idx, jint options)
  {
    ActiveMapsLayout & layout = storage_utils::GetMapLayout();
    LocalAndRemoteSizeT const sizes = layout.GetRemoteCountrySizes(ToNative(idx));
    switch (storage_utils::ToOptions(options))
    {
      case MapOptions::Map:
        return sizes.first;
      case MapOptions::CarRouting:
        return sizes.second;
      case MapOptions::MapWithCarRouting:
        return sizes.first + sizes.second;
      case MapOptions::Nothing:
        return 0;
    }
  }

  JNIEXPORT jint JNICALL
  Java_com_mapswithme_maps_MapStorage_countryStatus(JNIEnv * env, jobject thiz, jobject idx)
  {
    return static_cast<jint>(g_framework->GetCountryStatus(IndexBinding(idx).toNative()));
  }

  JNIEXPORT jobject JNICALL
  Java_com_mapswithme_maps_MapStorage_findIndexByFile(JNIEnv * env, jobject thiz, jstring name)
  {
    char const * s = env->GetStringUTFChars(name, 0);
    if (s == 0)
      return 0;

    TIndex const idx = GetStorage().FindIndexByFile(s);
    if (idx.IsValid())
      return ToJava(idx);
    else
      return 0;
  }

  void ReportChangeCountryStatus(shared_ptr<jobject> const & obj, TIndex const & idx)
  {
    JNIEnv * env = jni::GetEnv();

    jmethodID methodID = jni::GetJavaMethodID(env, *obj.get(), "onCountryStatusChanged", "(Lcom/mapswithme/maps/MapStorage$Index;)V");
    env->CallVoidMethod(*obj.get(), methodID, ToJava(idx));
  }

  void ReportCountryProgress(shared_ptr<jobject> const & obj, TIndex const & idx, pair<int64_t, int64_t> const & p)
  {
    jlong const current = p.first;
    jlong const total = p.second;

    JNIEnv * env = jni::GetEnv();

    jmethodID methodID = jni::GetJavaMethodID(env, *obj.get(), "onCountryProgress", "(Lcom/mapswithme/maps/MapStorage$Index;JJ)V");
    env->CallVoidMethod(*obj.get(), methodID, ToJava(idx), current, total);
  }

  JNIEXPORT jint JNICALL
  Java_com_mapswithme_maps_MapStorage_subscribe(JNIEnv * env, jobject thiz, jobject obj)
  {
    LOG(LDEBUG, ("Subscribe on storage"));

    return GetStorage().Subscribe(bind(&ReportChangeCountryStatus, jni::make_global_ref(obj), _1),
                               bind(&ReportCountryProgress, jni::make_global_ref(obj), _1, _2));
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_MapStorage_unsubscribe(JNIEnv * env, jobject thiz, jint slotID)
  {
    LOG(LDEBUG, ("UnSubscribe from storage"));

    GetStorage().Unsubscribe(slotID);
  }

  JNIEXPORT jboolean JNICALL
  Java_com_mapswithme_maps_MapStorage_nativeMoveFile(JNIEnv * env, jobject thiz, jstring oldFile, jstring newFile)
  {
    return my::RenameFileX(jni::ToNativeString(env, oldFile), jni::ToNativeString(env, newFile));
  }
}

namespace storage
{
  jobject ToJava(TIndex const & idx)
  {
    JNIEnv * env = jni::GetEnv();

    jclass const klass = g_indexClazz;
    ASSERT(klass, ());

    jmethodID static const methodID = env->GetMethodID(klass, "<init>", "(III)V");
    ASSERT(methodID, ());

    return env->NewObject(klass, methodID,
                          static_cast<jint>(idx.m_group),
                          static_cast<jint>(idx.m_country),
                          static_cast<jint>(idx.m_region));
  }

  TIndex ToNative(jobject idx)
  {
    return IndexBinding(idx).toNative();
  }
}

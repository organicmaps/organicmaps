#include "android/jni/com/mapswithme/maps/guides/Guides.hpp"

#include "android/jni/com/mapswithme/platform/Platform.hpp"

namespace
{
jclass g_galleryClass = nullptr;
jclass g_itemClass = nullptr;
jclass g_cityParamsClass = nullptr;
jclass g_outdoorParamsClass = nullptr;
jmethodID g_galleryConstructor = nullptr;
jmethodID g_itemConstructor = nullptr;
jmethodID g_cityParamsConstructor = nullptr;
jmethodID g_outdoorParamsConstructor = nullptr;

void PrepareClassRefs(JNIEnv *env)
{
  if (g_galleryClass != nullptr)
    return;

  g_galleryClass = jni::GetGlobalClassRef(env, "com/mapswithme/maps/guides/GuidesGallery");
  g_galleryConstructor = jni::GetConstructorID(env, g_galleryClass,
                                               "([Lcom/mapswithme/maps/guides/GuidesGallery$Item;)V");
  g_itemClass = jni::GetGlobalClassRef(env, "com/mapswithme/maps/guides/GuidesGallery$Item");
  // public Item(@NonNull String guideId, @NonNull String url, @NonNull String imageUrl,
  //             @NonNull String title, @NonNull String subTitle, int type,
  //             boolean downloaded, @Nullable CityParams cityParams,
  //             @Nullable OutdoorParams outdoorParams)
  g_itemConstructor
    = jni::GetConstructorID(env, g_itemClass, "(Ljava/lang/String;Ljava/lang/String;"
                                              "Ljava/lang/String;Ljava/lang/String;"
                                              "IZLcom/mapswithme/maps/guides/GuidesGallery$CityParams;"
                                              "Lcom/mapswithme/maps/guides/GuidesGallery$OutdoorParams;)"
                                              "V");
  g_cityParamsClass
    = jni::GetGlobalClassRef(env, "com/mapswithme/maps/guides/GuidesGallery$CityParams");
  // public CityParams(int bookmarksCount, boolean isTrackAvailable)
  g_cityParamsConstructor = jni::GetConstructorID(env, g_cityParamsClass, "(IZ)V");
  g_outdoorParamsClass
    = jni::GetGlobalClassRef(env, "com/mapswithme/maps/guides/GuidesGallery$OutdoorParams");
  // public OutdoorParams(double distance, long duration, int ascent)
  g_outdoorParamsConstructor
    = jni::GetConstructorID(env, g_outdoorParamsClass, "(Ljava/lang/String;DJI)V");
  jni::HandleJavaException(env);
}
} // namespace

namespace guides
{
jobject CreateGallery(JNIEnv *env, GuidesManager::GuidesGallery const & gallery)
{
  PrepareClassRefs(env);

  auto const itemBuilder = [](JNIEnv *env, GuidesManager::GuidesGallery::Item const & item)
  {
    jni::TScopedLocalRef guideId(env, jni::ToJavaString(env, item.m_guideId));
    jni::TScopedLocalRef url(env, jni::ToJavaString(env, item.m_url));
    jni::TScopedLocalRef imageUrl(env, jni::ToJavaString(env, item.m_imageUrl));
    jni::TScopedLocalRef title(env, jni::ToJavaString(env, item.m_title));
    auto const type = static_cast<jint>(item.m_type);
    auto const downloaded = static_cast<jboolean>(item.m_downloaded);
    jni::TScopedLocalRef cityParams(env, nullptr);
    jni::TScopedLocalRef outdoorParams(env, nullptr);
    if (item.m_type == GuidesManager::GuidesGallery::Item::Type::City)
    {
      cityParams.reset(env->NewObject(g_cityParamsClass, g_cityParamsConstructor,
                                      static_cast<jint>(item.m_cityParams.m_bookmarksCount),
                                      static_cast<jboolean>(item.m_cityParams.m_trackIsAvailable)));
    } else if (item.m_type == GuidesManager::GuidesGallery::Item::Type::Outdoor)
    {
      outdoorParams.reset(env->NewObject(g_outdoorParamsClass, g_outdoorParamsConstructor,
                                         jni::ToJavaString(env, item.m_outdoorsParams.m_tag),
                                         static_cast<jdouble>(item.m_outdoorsParams.m_distance),
                                         static_cast<jlong>(item.m_outdoorsParams.m_duration),
                                         static_cast<jint>(item.m_outdoorsParams.m_ascent)));
    }

    return env->NewObject(g_itemClass, g_itemConstructor, guideId.get(), url.get(), imageUrl.get(),
                          title.get(), type, downloaded, cityParams.get(),
                          outdoorParams.get());
  };

  jni::TScopedLocalObjectArrayRef items(env, jni::ToJavaArray(env, g_itemClass, gallery.m_items,
                                        itemBuilder));
  return env->NewObject(g_galleryClass, g_galleryConstructor, items.get());
}
} // namespace guides

namespace platform
{
bool IsGuidesLayerFirstLaunch()
{
  JNIEnv * env = jni::GetEnv();
  static const jclass sharedPropertiesClass = jni::GetGlobalClassRef(env, "com/mapswithme/util/SharedPropertiesUtils");
  static const jmethodID getter = jni::GetStaticMethodID(env, sharedPropertiesClass,
                                                         "shouldShowNewMarkerForLayerMode",
                                                         "(Landroid/content/Context;Ljava/lang/String;)Z");
  jobject context = android::Platform::Instance().GetContext();
  jni::ScopedLocalRef mode(env, jni::ToJavaString(env, "GUIDES"));

  return env->CallStaticBooleanMethod(sharedPropertiesClass, getter, context, mode.get());
}

void SetGuidesLayerFirstLaunch(bool /* isFirstLaunch */)
{
  JNIEnv * env = jni::GetEnv();
  static const jclass sharedPropertiesClass = jni::GetGlobalClassRef(env, "com/mapswithme/util/SharedPropertiesUtils");
  static const jmethodID setter = jni::GetStaticMethodID(env, sharedPropertiesClass,
                                                         "setLayerMarkerShownForLayerMode",
                                                         "(Landroid/content/Context;Ljava/lang/String;)V");
  jobject context = android::Platform::Instance().GetContext();
  jni::ScopedLocalRef mode(env, jni::ToJavaString(env, "GUIDES"));

  env->CallStaticVoidMethod(sharedPropertiesClass, setter, context, mode.get());
}
}  // namespace platform

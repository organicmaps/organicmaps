#include "UserMarkHelper.hpp"

#include "map/place_page_info.hpp"

namespace usermark_helper
{
using search::AddressInfo;
using feature::Metadata;

void InjectMetadata(JNIEnv * env, jclass const clazz, jobject const mapObject, feature::Metadata const & metadata)
{
  static jmethodID const addId = env->GetMethodID(clazz, "addMetadata", "(ILjava/lang/String;)V");
  ASSERT(addId, ());

  for (auto const t : metadata.GetPresentTypes())
  {
    // TODO: It is not a good idea to pass raw strings to UI. Calling separate getters should be a better way.
    jni::TScopedLocalRef metaString(env, t == feature::Metadata::FMD_WIKIPEDIA ?
                                              jni::ToJavaString(env, metadata.GetWikiURL()) :
                                              jni::ToJavaString(env, metadata.Get(t)));
    env->CallVoidMethod(mapObject, addId, t, metaString.get());
  }
}

pair<jintArray, jobjectArray> NativeMetadataToJavaMetadata(JNIEnv * env, Metadata const & metadata)
{
  auto const metaTypes = metadata.GetPresentTypes();
  const jintArray j_metaTypes = env->NewIntArray(metadata.Size());
  jint * arr = env->GetIntArrayElements(j_metaTypes, 0);
  const jobjectArray j_metaValues = env->NewObjectArray(metadata.Size(), jni::GetStringClass(env), 0);

  for (size_t i = 0; i < metaTypes.size(); i++)
  {
    auto const type = metaTypes[i];
    arr[i] = type;
    // TODO: Refactor code to use separate getters for each metadata.
    jni::TScopedLocalRef metaString(env, type == Metadata::FMD_WIKIPEDIA ?
                                                 jni::ToJavaString(env, metadata.GetWikiURL()) :
                                                 jni::ToJavaString(env, metadata.Get(type)));
    env->SetObjectArrayElement(j_metaValues, i, metaString.get());
  }
  env->ReleaseIntArrayElements(j_metaTypes, arr, 0);

  return make_pair(j_metaTypes, j_metaValues);
}

// TODO(yunikkk): displayed information does not need separate street and house.
// There is an AddressInfo::FormatAddress() method which should be used instead.
jobject CreateMapObject(JNIEnv * env, int mapObjectType, string const & name, double lat, double lon, string const & typeName, string const & street, string const & house, Metadata const & metadata)
{
  // Java signature :
  // public MapObject(@MapObjectType int mapObjectType, String name, double lat, double lon, String typeName, String street, String house)
  static jmethodID const ctorId =
      jni::GetConstructorID(env, g_mapObjectClazz, "(ILjava/lang/String;DDLjava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");

  jobject mapObject = env->NewObject(g_mapObjectClazz, ctorId,
                                     static_cast<jint>(mapObjectType),
                                     jni::ToJavaString(env, name),
                                     static_cast<jdouble>(lat),
                                     static_cast<jdouble>(lon),
                                     jni::ToJavaString(env, typeName),
                                     jni::ToJavaString(env, street),
                                     jni::ToJavaString(env, house));

  InjectMetadata(env, g_mapObjectClazz, mapObject, metadata);
  return mapObject;
}

jobject CreateMapObject(JNIEnv * env, place_page::Info const & info)
{
  ms::LatLon const ll = info.GetLatLon();

  if (info.IsMyPosition())
    return CreateMapObject(env, kMyPosition, {}, ll.lat, ll.lon, {}, {}, {}, {});

  // TODO(yunikkk): object can be POI + API + search result + bookmark simultaneously.
  if (info.HasApiUrl())
    return CreateMapObject(env, kApiPoint, info.GetTitle(), ll.lat, ll.lon, info.GetSubtitle(), {}, {}, info.GetMetadata());

  // TODO(yunikkk): Bookmark can also be a feature.
  if (info.IsBookmark())
  {
    // Java signature :
    // public Bookmark(@IntRange(from = 0) int categoryId, @IntRange(from = 0) int bookmarkId, String name)
    static jmethodID const ctorId = jni::GetConstructorID(env, g_bookmarkClazz, "(IILjava/lang/String;)V");
    jni::TScopedLocalRef jName(env, jni::ToJavaString(env, info.GetTitle()));
    jobject mapObject = env->NewObject(g_bookmarkClazz, ctorId,
                                       static_cast<jint>(info.m_bac.first),
                                       static_cast<jint>(info.m_bac.second),
                                       jName.get());
    if (info.IsFeature())
      InjectMetadata(env, g_mapObjectClazz, mapObject, info.GetMetadata());
    return mapObject;
  }

  Framework * frm = g_framework->NativeFramework();
  if (info.IsFeature())
  {
    search::AddressInfo const address = frm->GetFeatureAddressInfo(info.GetID());
    // TODO(yunikkk): Pass address.FormatAddress() to UI instead of separate house and street.
    // TODO(yunikkk): Should we pass localized strings here and in other methods as byte arrays?
    return CreateMapObject(env, kPoi, info.GetTitle(), ll.lat, ll.lon, info.GetSubtitle(), address.m_street,
                           address.m_house, info.GetMetadata());
  }
  // User have selected an empty place on a map with a long tap.
  return CreateMapObject(env, kPoi, info.GetTitle(), ll.lat, ll.lon, info.GetSubtitle(), {}, {}, {});
}
}  // namespace usermark_helper

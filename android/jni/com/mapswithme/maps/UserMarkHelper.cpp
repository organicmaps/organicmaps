#include "UserMarkHelper.hpp"

namespace usermark_helper
{
using search::AddressInfo;
using feature::Metadata;

template <class T>
T const * CastMark(UserMark const * data)
{
  return static_cast<T const *>(data);
}

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

void FillAddressAndMetadata(UserMark const * mark, AddressInfo & info, Metadata & metadata)
{
  Framework * frm = g_framework->NativeFramework();
  auto * feature = mark->GetFeature();
  if (feature)
  {
    info = frm->GetFeatureAddressInfo(*feature);
    metadata = feature->GetMetadata();
  }
  else
  {
    // Calculate at least country name for a point. Can we provide more address information?
    info.m_country = frm->GetCountryName(mark->GetPivot());
  }
}

jobject CreateBookmark(int categoryId, int bookmarkId, string const & typeName, Metadata const & metadata)
{
  JNIEnv * env = jni::GetEnv();
  // Java signature :
  // public Bookmark(@IntRange(from = 0) int categoryId, @IntRange(from = 0) int bookmarkId, String name)
  static jmethodID const ctorId = jni::GetConstructorID(env, g_bookmarkClazz, "(IILjava/lang/String;)V");
  jni::TScopedLocalRef name(env, jni::ToJavaString(env, typeName));
  jobject mapObject = env->NewObject(g_bookmarkClazz, ctorId,
                                     static_cast<jint>(categoryId),
                                     static_cast<jint>(bookmarkId),
                                     name.get());

  InjectMetadata(env, g_mapObjectClazz, mapObject, metadata);
  return mapObject;
}

jobject CreateMapObject(int mapObjectType, string const & name, double lat, double lon, string const & typeName, string const & street, string const & house, Metadata const & metadata)
{
  JNIEnv * env = jni::GetEnv();
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

jobject CreateMapObject(UserMark const * userMark)
{
  search::AddressInfo info;
  feature::Metadata metadata;
  FillAddressAndMetadata(userMark, info, metadata);
  jobject mapObject = nullptr;
  ms::LatLon ll = userMark->GetLatLon();
  switch (userMark->GetMarkType())
  {
    case UserMark::Type::API:
    {
      ApiMarkPoint const * apiMark = CastMark<ApiMarkPoint>(userMark);
      mapObject = CreateMapObject(kApiPoint, apiMark->GetName(), ll.lat, ll.lon, apiMark->GetID(), "", "", metadata);
      break;
    }
    case UserMark::Type::BOOKMARK:
    {
      BookmarkAndCategory bmAndCat = g_framework->NativeFramework()->FindBookmark(userMark);
      Bookmark const * bookmark = CastMark<Bookmark>(userMark);
      if (IsValid(bmAndCat))
        mapObject = CreateBookmark(bmAndCat.first, bmAndCat.second, bookmark->GetName(), metadata);
      break;
    }
    case UserMark::Type::POI:
    {
      // TODO(AlexZ): Refactor out passing custom name for shared links.
      auto const & cn = CastMark<PoiMarkPoint>(userMark)->GetCustomName();
      if (!cn.empty())
        info.m_name = cn;
      mapObject = CreateMapObject(kPoi, info.GetPinName(), ll.lat, ll.lon, info.GetPinType(), info.m_street, info.m_house, metadata);
      break;
    }
    case UserMark::Type::SEARCH:
    {
      mapObject = CreateMapObject(kSearch, info.GetPinName(), ll.lat, ll.lon, info.GetPinType(), info.m_street, info.m_house, metadata);
      break;
    }
    case UserMark::Type::MY_POSITION:
    {
      mapObject = CreateMapObject(kMyPosition, "", ll.lat, ll.lon, "", "", "", metadata);
      break;
    }
    case UserMark::Type::DEBUG_MARK:
    {
      // Ignore clicks to debug marks.
      break;
    }
  }

  return mapObject;
}
} // namespace usermark_helper

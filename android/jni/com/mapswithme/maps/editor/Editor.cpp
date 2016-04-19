#include <jni.h>

#include "com/mapswithme/core/jni_helper.hpp"
#include "com/mapswithme/maps/Framework.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include "indexer/cuisines.hpp"
#include "indexer/editable_map_object.hpp"
#include "indexer/osm_editor.hpp"

#include "std/algorithm.hpp"
#include "std/set.hpp"
#include "std/target_os.hpp"
#include "std/vector.hpp"

namespace
{
using TCuisine = pair<string, string>;
osm::EditableMapObject g_editableMapObject;

jclass g_featureCategoryClazz;
jmethodID g_featureCtor;

jobject ToJavaFeatureCategory(JNIEnv * env, osm::Category const & category)
{
  return env->NewObject(g_featureCategoryClazz, g_featureCtor, category.m_type, jni::TScopedLocalRef(env, jni::ToJavaString(env, category.m_name)).get());
}

jobjectArray ToJavaArray(JNIEnv * env, vector<string> const & src)
{
  int const size = src.size();
  auto jArray = env->NewObjectArray(size, jni::GetStringClass(env), 0);
  for (size_t i = 0; i < size; i++)
  {
    jni::TScopedLocalRef jItem(env, jni::ToJavaString(env, src[i]));
    env->SetObjectArrayElement(jArray, i, jItem.get());
  }

  return jArray;
}
}  // namespace

extern "C"
{
using osm::Editor;

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeInit(JNIEnv * env, jclass)
{
  g_featureCategoryClazz = jni::GetGlobalClassRef(env, "com/mapswithme/maps/editor/data/FeatureCategory");;
  // public FeatureCategory(int category, String name)
  g_featureCtor = jni::GetConstructorID(env, g_featureCategoryClazz, "(ILjava/lang/String;)V");
}

JNIEXPORT jstring JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeGetOpeningHours(JNIEnv * env, jclass)
{
  return jni::ToJavaString(env, g_editableMapObject.GetOpeningHours());
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeSetOpeningHours(JNIEnv * env, jclass, jstring value)
{
  g_editableMapObject.SetOpeningHours(jni::ToNativeString(env, value));
}

JNIEXPORT jstring JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeGetPhone(JNIEnv * env, jclass)
{
  return jni::ToJavaString(env, g_editableMapObject.GetPhone());
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeSetPhone(JNIEnv * env, jclass, jstring value)
{
  g_editableMapObject.SetPhone(jni::ToNativeString(env, value));
}

JNIEXPORT jstring JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeGetWebsite(JNIEnv * env, jclass)
{
  return jni::ToJavaString(env, g_editableMapObject.GetWebsite());
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeSetWebsite(JNIEnv * env, jclass, jstring value)
{
  g_editableMapObject.SetWebsite(jni::ToNativeString(env, value));
}

JNIEXPORT jstring JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeGetEmail(JNIEnv * env, jclass)
{
  return jni::ToJavaString(env, g_editableMapObject.GetEmail());
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeSetEmail(JNIEnv * env, jclass, jstring value)
{
  g_editableMapObject.SetEmail(jni::ToNativeString(env, value));
}

JNIEXPORT jint JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeGetStars(JNIEnv * env, jclass)
{
  return g_editableMapObject.GetStars();
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeSetStars(JNIEnv * env, jclass, jint value)
{
  g_editableMapObject.SetStars(value);
}

JNIEXPORT jstring JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeGetOperator(JNIEnv * env, jclass)
{
  return jni::ToJavaString(env, g_editableMapObject.GetOperator());
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeSetOperator(JNIEnv * env, jclass, jstring value)
{
  g_editableMapObject.SetOperator(jni::ToNativeString(env, value));
}

JNIEXPORT jdouble JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeGetElevation(JNIEnv * env, jclass)
{
  double elevation;
  return g_editableMapObject.GetElevation(elevation) ? elevation : -1;
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeSetElevation(JNIEnv * env, jclass, jdouble value)
{
  g_editableMapObject.SetElevation(value);
}

JNIEXPORT jstring JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeGetWikipedia(JNIEnv * env, jclass)
{
  return jni::ToJavaString(env, g_editableMapObject.GetWikipedia());
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeSetWikipedia(JNIEnv * env, jclass, jstring value)
{
  g_editableMapObject.SetWikipedia(jni::ToNativeString(env, value));
}

JNIEXPORT jstring JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeGetFlats(JNIEnv * env, jclass)
{
  return jni::ToJavaString(env, g_editableMapObject.GetFlats());
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeSetFlats(JNIEnv * env, jclass, jstring value)
{
  g_editableMapObject.SetFlats(jni::ToNativeString(env, value));
}

JNIEXPORT jstring JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeGetBuildingLevels(JNIEnv * env, jclass)
{
  return jni::ToJavaString(env, g_editableMapObject.GetBuildingLevels());
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeSetBuildingLevels(JNIEnv * env, jclass, jstring value)
{
  g_editableMapObject.SetBuildingLevels(jni::ToNativeString(env, value));
}

JNIEXPORT jstring JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeGetZipCode(JNIEnv * env, jclass)
{
  return jni::ToJavaString(env, g_editableMapObject.GetPostcode());
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeSetZipCode(JNIEnv * env, jclass clazz, jstring value)
{
  g_editableMapObject.SetPostcode(jni::ToNativeString(env, value));
}

JNIEXPORT jboolean JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeHasWifi(JNIEnv *, jclass)
{
  return g_editableMapObject.GetInternet() == osm::Internet::Wlan;
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeSetHasWifi(JNIEnv *, jclass, jboolean hasWifi)
{
  g_editableMapObject.SetInternet(osm::Internet::Wlan);
}

JNIEXPORT jboolean JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeSaveEditedFeature(JNIEnv *, jclass)
{
  switch (g_framework->NativeFramework()->SaveEditedMapObject(g_editableMapObject))
  {
  case osm::Editor::NothingWasChanged:
  case osm::Editor::SavedSuccessfully:
    return true;
  case osm::Editor::NoFreeSpaceError:
    return false;
  }
}

JNIEXPORT jboolean JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeIsFeatureEditable(JNIEnv *, jclass)
{
  return g_framework->GetPlacePageInfo().IsEditable();
}

JNIEXPORT jintArray JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeGetEditableFields(JNIEnv * env, jclass clazz)
{
  auto const & editable = g_editableMapObject.GetEditableFields();
  int const size = editable.size();
  jintArray jEditableFields = env->NewIntArray(size);
  jint * arr = env->GetIntArrayElements(jEditableFields, 0);
  for (int i = 0; i < size; ++i)
    arr[i] = editable[i];
  env->ReleaseIntArrayElements(jEditableFields, arr, 0);

  return jEditableFields;
}

JNIEXPORT jboolean JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeIsAddressEditable(JNIEnv * env, jclass clazz)
{
  return g_editableMapObject.IsAddressEditable();
}

JNIEXPORT jboolean JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeIsNameEditable(JNIEnv * env, jclass clazz)
{
  return g_editableMapObject.IsNameEditable();
}

JNIEXPORT jstring JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeGetDefaultName(JNIEnv * env, jclass)
{
  // TODO(yunikkk): add multilanguage names support via EditableMapObject::GetLocalizedName().
  return jni::ToJavaString(env, g_editableMapObject.GetDefaultName());
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeSetDefaultName(JNIEnv * env, jclass, jstring name)
{
  // TODO(yunikkk): add multilanguage names support.
  g_editableMapObject.SetName(jni::ToNativeString(env, name), StringUtf8Multilang::kDefaultCode);
}

JNIEXPORT jstring JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeGetStreet(JNIEnv * env, jclass)
{
  // TODO(yunikkk): use "GetStreet().m_localizedName" to get localized street
  return jni::ToJavaString(env, g_editableMapObject.GetStreet().m_defaultName);
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeSetStreet(JNIEnv * env, jclass, jstring street)
{
  //TODO(yunikkk): pass original localized street name as a second parameter.
  g_editableMapObject.SetStreet({jni::ToNativeString(env, street), ""});
}

JNIEXPORT jstring JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeGetHouseNumber(JNIEnv * env, jclass)
{
  return jni::ToJavaString(env, g_editableMapObject.GetHouseNumber());
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeSetHouseNumber(JNIEnv * env, jclass, jstring houseNumber)
{
  g_editableMapObject.SetHouseNumber(jni::ToNativeString(env, houseNumber));
}

JNIEXPORT jobjectArray JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeGetNearbyStreets(JNIEnv * env, jclass clazz)
{
  auto const & streets = g_editableMapObject.GetNearbyStreets();
  int const size = streets.size();
  jobjectArray jStreets = env->NewObjectArray(size, jni::GetStringClass(env), 0);
  for (int i = 0; i < size; ++i)
    // TODO(yunikkk): use "streets[i].m_localizedName" to get localized street.
    env->SetObjectArrayElement(jStreets, i, jni::TScopedLocalRef(env, jni::ToJavaString(env, streets[i].m_defaultName)).get());
  return jStreets;
}

JNIEXPORT jboolean JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeHasSomethingToUpload(JNIEnv * env, jclass clazz)
{
  return Editor::Instance().HaveMapEditsOrNotesToUpload();
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeUploadChanges(JNIEnv * env, jclass clazz, jstring token, jstring secret,
    jstring appVersion, jstring appId)
{
  // TODO: Handle upload status in callback
  Editor::Instance().UploadChanges(jni::ToNativeString(env, token), jni::ToNativeString(env, secret),
      {{"created_by", "MAPS.ME " OMIM_OS_NAME " " + jni::ToNativeString(env, appVersion)},
       {"bundle_id", jni::ToNativeString(env, appId)}}, nullptr);
}

JNIEXPORT jlongArray JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeGetStats(JNIEnv * env, jclass clazz)
{
  auto const stats = Editor::Instance().GetStats();
  jlongArray result = env->NewLongArray(3);
  jlong buf[] = {stats.m_edits.size(), stats.m_uploadedCount, stats.m_lastUploadTimestamp};
  env->SetLongArrayRegion(result, 0, 3, buf);
  return result;
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeClearLocalEdits(JNIEnv * env, jclass clazz)
{
  Editor::Instance().ClearAllLocalEdits();
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeStartEdit(JNIEnv *, jclass)
{
  ::Framework * frm = g_framework->NativeFramework();
  place_page::Info const & info = g_framework->GetPlacePageInfo();
  CHECK(frm->GetEditableMapObject(info.GetID(), g_editableMapObject), ("Invalid feature in the place page."));
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeCreateMapObject(JNIEnv *, jclass, jint featureCategory)
{
  ::Framework * frm = g_framework->NativeFramework();
  CHECK(frm->CreateMapObject(frm->GetViewportCenter(), featureCategory, g_editableMapObject),
        ("Couldn't create mapobject, wrong coordinates of missing mwm"));
}

JNIEXPORT jobjectArray JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeGetNewFeatureCategories(JNIEnv * env, jclass clazz)
{
  osm::NewFeatureCategories const & printableTypes = g_framework->NativeFramework()->GetEditorCategories();
  int const size = printableTypes.m_allSorted.size();
  auto jCategories = env->NewObjectArray(size, g_featureCategoryClazz, 0);
  for (size_t i = 0; i < size; i++)
  {
    // TODO pass used categories section, too
    jni::TScopedLocalRef jCategory(env, ToJavaFeatureCategory(env, printableTypes.m_allSorted[i]));
    env->SetObjectArrayElement(jCategories, i, jCategory.get());
  }

  return jCategories;
}

JNIEXPORT jobjectArray JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeGetCuisines(JNIEnv * env, jclass clazz)
{
  osm::TAllCuisines const & cuisines = osm::Cuisines::Instance().AllSupportedCuisines();
  vector<string> keys;
  keys.reserve(cuisines.size());
  for (TCuisine const & cuisine : cuisines)
    keys.push_back(cuisine.first);
  return ToJavaArray(env, keys);
}

JNIEXPORT jobjectArray JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeGetSelectedCuisines(JNIEnv * env, jclass clazz)
{
  return ToJavaArray(env, g_editableMapObject.GetCuisines());
}

JNIEXPORT jobjectArray JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeTranslateCuisines(JNIEnv * env, jclass clazz, jobjectArray jKeys)
{
  int const length = env->GetArrayLength(jKeys);
  vector<string> translations;
  translations.reserve(length);
  for (int i = 0; i < length; i++)
  {
    string const & key = jni::ToNativeString(env, (jstring) env->GetObjectArrayElement(jKeys, i));
    translations.push_back(osm::Cuisines::Instance().Translate(key));
  }
  return ToJavaArray(env, translations);
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeSetSelectedCuisines(JNIEnv * env, jclass clazz, jobjectArray jKeys)
{
  int const length = env->GetArrayLength(jKeys);
  vector<string> cuisines;
  cuisines.reserve(length);
  for (int i = 0; i < length; i++)
    cuisines.push_back(jni::ToNativeString(env, (jstring) env->GetObjectArrayElement(jKeys, i)));
  g_editableMapObject.SetCuisines(cuisines);
}

JNIEXPORT jstring JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeGetFormattedCuisine(JNIEnv * env, jclass clazz)
{
  return jni::ToJavaString(env, g_editableMapObject.FormatCuisines());
}

JNIEXPORT jstring JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeGetMwmName(JNIEnv * env, jclass clazz)
{
  return jni::ToJavaString(env, g_editableMapObject.GetID().GetMwmName());
}

JNIEXPORT jlong JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeGetMwmVersion(JNIEnv * env, jclass clazz)
{
  return g_editableMapObject.GetID().GetMwmVersion();
}

// static void nativeCreateNote(double lat, double lon, String text);
JNIEXPORT void JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeCreateNote(JNIEnv * env, jclass clazz, jdouble lat, jdouble lon, jstring text)
{
  Editor::Instance().CreateNote(ms::LatLon(lat, lon), g_editableMapObject.GetID(),
                                osm::Editor::NoteProblemType::General, jni::ToNativeString(env, text));
}

// static void nativePlaceDoesNotExist(double lat, double lon);
JNIEXPORT void JNICALL
Java_com_mapswithme_maps_editor_Editor_nativePlaceDoesNotExist(JNIEnv * env, jclass clazz, jdouble lat, jdouble lon)
{
  Editor::Instance().CreateNote(ms::LatLon(lat, lon), g_editableMapObject.GetID(),
                                osm::Editor::NoteProblemType::PlaceDoesNotExist, "");
}

// static boolean nativeIsHouseValid(String houseNumber);
JNIEXPORT jboolean JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeIsHouseValid(JNIEnv * env, jclass clazz, jstring houseNumber)
{
  return osm::EditableMapObject::ValidateHouseNumber(jni::ToNativeString(env, houseNumber));
}

JNIEXPORT jstring JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeGetCategory(JNIEnv * env, jclass clazz)
{
  return jni::ToJavaString(env, g_editableMapObject.GetLocalizedType());
}
} // extern "C"

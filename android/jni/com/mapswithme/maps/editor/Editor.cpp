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
jclass g_localNameClazz;
jmethodID g_localNameCtor;
jfieldID g_localNameFieldCode;
jfieldID g_localNameFieldName;
jclass g_localStreetClazz;
jmethodID g_localStreetCtor;
jfieldID g_localStreetFieldDef;
jfieldID g_localStreetFieldLoc;

jobject ToJavaFeatureCategory(JNIEnv * env, osm::Category const & category)
{
  return env->NewObject(g_featureCategoryClazz, g_featureCtor, category.m_type, jni::TScopedLocalRef(env, jni::ToJavaString(env, category.m_name)).get());
}

jobject ToJavaName(JNIEnv * env, osm::LocalizedName const & name)
{
  jni::TScopedLocalRef jName(env, jni::ToJavaString(env, name.m_name));
  jni::TScopedLocalRef jLang(env, jni::ToJavaString(env, name.m_lang));
  jni::TScopedLocalRef jLangName(env, jni::ToJavaString(env, name.m_langName));
  return env->NewObject(g_localNameClazz, g_localNameCtor, name.m_code,
                        jName.get(), jLang.get(), jLangName.get());
}

jobject ToJavaStreet(JNIEnv * env, osm::LocalizedStreet const & street)
{
  return env->NewObject(g_localStreetClazz, g_localStreetCtor,
                        jni::TScopedLocalRef(env, jni::ToJavaString(env, street.m_defaultName)).get(),
                        jni::TScopedLocalRef(env, jni::ToJavaString(env, street.m_localizedName)).get());
}
}  // namespace

extern "C"
{
using osm::Editor;

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeInit(JNIEnv * env, jclass)
{
  g_featureCategoryClazz = jni::GetGlobalClassRef(env, "com/mapswithme/maps/editor/data/FeatureCategory");
  // FeatureCategory(int category, String name)
  g_featureCtor = jni::GetConstructorID(env, g_featureCategoryClazz, "(ILjava/lang/String;)V");

  g_localNameClazz = jni::GetGlobalClassRef(env, "com/mapswithme/maps/editor/data/LocalizedName");
  // LocalizedName(int code, @NonNull String name, @NonNull String lang, @NonNull String langName)
  g_localNameCtor = jni::GetConstructorID(env, g_localNameClazz, "(ILjava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");
  g_localNameFieldCode = env->GetFieldID(g_localNameClazz, "code", "I");
  g_localNameFieldName = env->GetFieldID(g_localNameClazz, "name", "Ljava/lang/String;");

  g_localStreetClazz = jni::GetGlobalClassRef(env, "com/mapswithme/maps/editor/data/LocalizedStreet");
  // LocalizedStreet(@NonNull String defaultName, @NonNull String localizedName)
  g_localStreetCtor = jni::GetConstructorID(env, g_localStreetClazz, "(Ljava/lang/String;Ljava/lang/String;)V");
  g_localStreetFieldDef = env->GetFieldID(g_localStreetClazz, "defaultName", "Ljava/lang/String;");
  g_localStreetFieldLoc = env->GetFieldID(g_localStreetClazz, "localizedName", "Ljava/lang/String;");
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
  g_editableMapObject.SetInternet(hasWifi ? osm::Internet::Wlan : osm::Internet::Unknown);
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

JNIEXPORT jboolean JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeIsBuilding(JNIEnv * env, jclass clazz)
{
  return g_editableMapObject.IsBuilding();
}

JNIEXPORT jstring JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeGetDefaultName(JNIEnv * env, jclass)
{
  return jni::ToJavaString(env, g_editableMapObject.GetDefaultName());
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeSetDefaultName(JNIEnv * env, jclass, jstring name)
{
  g_editableMapObject.SetName(jni::ToNativeString(env, name), StringUtf8Multilang::kDefaultCode);
}

JNIEXPORT jobjectArray JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeGetLocalizedNames(JNIEnv * env, jclass)
{
  return jni::ToJavaArray(env, g_localNameClazz, g_editableMapObject.GetLocalizedNames(), ToJavaName);
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeSetLocalizedNames(JNIEnv * env, jclass, jobjectArray names)
{
  int const length = env->GetArrayLength(names);
  for (int i = 0; i < length; i++)
  {
    auto jName = env->GetObjectArrayElement(names, i);
    g_editableMapObject.SetName(jni::ToNativeString(env, static_cast<jstring>(env->GetObjectField(jName, g_localNameFieldName))),
                                env->GetIntField(jName, g_localNameFieldCode));
  }
}

JNIEXPORT jobject JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeGetStreet(JNIEnv * env, jclass)
{
  return ToJavaStreet(env, g_editableMapObject.GetStreet());
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeSetStreet(JNIEnv * env, jclass, jobject street)
{
  g_editableMapObject.SetStreet({jni::ToNativeString(env, (jstring) env->GetObjectField(street, g_localStreetFieldDef)),
                                 jni::ToNativeString(env, (jstring) env->GetObjectField(street, g_localStreetFieldLoc))});
}
JNIEXPORT jobjectArray JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeGetNearbyStreets(JNIEnv * env, jclass clazz)
{
  return jni::ToJavaArray(env, g_localStreetClazz, g_editableMapObject.GetNearbyStreets(), ToJavaStreet);
}

JNIEXPORT jobjectArray JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeGetSupportedLanguages(JNIEnv * env, jclass clazz)
{
  // TODO (yunikkk) implement
  //public Language(@NonNull String code, @NonNull String name)
//  static jclass const langClass = jni::GetGlobalClassRef(env, "com/mapswithme/maps/editor/data/Language");
//  static jmethodID const langCtor = jni::GetConstructorID(env, langClass, "(Ljava/lang/String;Ljava/lang/String;)V");

//  return jni::ToJavaArray(env, langClass, g_editableMapObject.GetBuildingLevels(), );
  return nullptr;
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
  return jni::ToJavaArray(env, g_featureCategoryClazz, g_framework->NativeFramework()->GetEditorCategories().m_allSorted,
                          ToJavaFeatureCategory);
}

JNIEXPORT jobjectArray JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeGetUsedFeatureCategories(JNIEnv * env, jclass clazz)
{
  return jni::ToJavaArray(env, g_featureCategoryClazz, g_framework->NativeFramework()->GetEditorCategories().m_lastUsed,
                          ToJavaFeatureCategory);
}

JNIEXPORT jobjectArray JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeGetCuisines(JNIEnv * env, jclass clazz)
{
  osm::TAllCuisines const & cuisines = osm::Cuisines::Instance().AllSupportedCuisines();
  vector<string> keys;
  keys.reserve(cuisines.size());
  for (TCuisine const & cuisine : cuisines)
    keys.push_back(cuisine.first);
  return jni::ToJavaStringArray(env, keys);
}

JNIEXPORT jobjectArray JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeGetSelectedCuisines(JNIEnv * env, jclass clazz)
{
  return jni::ToJavaStringArray(env, g_editableMapObject.GetCuisines());
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
  return jni::ToJavaStringArray(env, translations);
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

// static void nativeCreateNote(String text);
JNIEXPORT void JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeCreateNote(JNIEnv * env, jclass clazz, jstring text)
{
  Editor::Instance().CreateNote(g_editableMapObject.GetLatLon(), g_editableMapObject.GetID(),
                                osm::Editor::NoteProblemType::General, jni::ToNativeString(env, text));
}

// static void nativePlaceDoesNotExist();
JNIEXPORT void JNICALL
Java_com_mapswithme_maps_editor_Editor_nativePlaceDoesNotExist(JNIEnv * env, jclass clazz)
{
  Editor::Instance().CreateNote(g_editableMapObject.GetLatLon(), g_editableMapObject.GetID(),
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

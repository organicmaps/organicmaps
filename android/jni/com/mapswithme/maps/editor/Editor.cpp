#include <jni.h>

#include "com/mapswithme/core/jni_helper.hpp"
#include "com/mapswithme/maps/Framework.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/string_utils.hpp"
#include "indexer/editable_map_object.hpp"
#include "indexer/osm_editor.hpp"

#include "std/algorithm.hpp"
#include "std/set.hpp"
#include "std/vector.hpp"

namespace
{
osm::EditableMapObject gEditableMapObject;
}  // namespace

extern "C"
{
using osm::Editor;

JNIEXPORT jstring JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeGetMetadata(JNIEnv * env, jclass, jint type)
{
  // TODO(yunikkk): Switch to osm::Props enum instead of metadata, and use separate getters instead a generic one.
  return jni::ToJavaString(env, gEditableMapObject.GetMetadata().Get(static_cast<feature::Metadata::EType>(type)));
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeSetMetadata(JNIEnv * env, jclass clazz, jint type, jstring value)
{
  // TODO(yunikkk): I would recommend to use separate setters/getters for each metadata field.
  string const v = jni::ToNativeString(env, value);
  using feature::Metadata;
  switch (type)
  {
  // TODO(yunikkk): Pass cuisines in a separate setter via vector of osm untranslated keys (SetCuisines()).
  case Metadata::FMD_CUISINE: gEditableMapObject.SetRawOSMCuisines(v); break;
  case Metadata::FMD_OPEN_HOURS: gEditableMapObject.SetOpeningHours(v); break;
  case Metadata::FMD_PHONE_NUMBER: gEditableMapObject.SetPhone(v); break;
  case Metadata::FMD_FAX_NUMBER: gEditableMapObject.SetFax(v); break;
  case Metadata::FMD_STARS:
    {
      // TODO(yunikkk): Pass stars in a separate integer setter.
      int stars;
      if (strings::to_int(v, stars))
        gEditableMapObject.SetStars(stars);
      break;
    }
  case Metadata::FMD_OPERATOR: gEditableMapObject.SetOperator(v); break;
  case Metadata::FMD_URL:  // We don't allow url in UI. Website should be used instead.
  case Metadata::FMD_WEBSITE: gEditableMapObject.SetWebsite(v); break;
  case Metadata::FMD_INTERNET: // TODO(yunikkk): use separate setter for Internet.
    {
      osm::Internet inet = osm::Internet::Unknown;
      if (v == DebugPrint(osm::Internet::Wlan))
        inet = osm::Internet::Wlan;
      if (v == DebugPrint(osm::Internet::Wired))
        inet = osm::Internet::Wired;
      if (v == DebugPrint(osm::Internet::No))
        inet = osm::Internet::No;
      if (v == DebugPrint(osm::Internet::Yes))
        inet = osm::Internet::Yes;
      gEditableMapObject.SetInternet(inet);
    }
    break;
  case Metadata::FMD_ELE:
    {
      double ele;
      if (strings::to_double(v, ele))
        gEditableMapObject.SetElevation(ele);
      break;
    }
  case Metadata::FMD_EMAIL: gEditableMapObject.SetEmail(v); break;
  case Metadata::FMD_POSTCODE: gEditableMapObject.SetPostcode(v); break;
  case Metadata::FMD_WIKIPEDIA: gEditableMapObject.SetWikipedia(v); break;
  case Metadata::FMD_FLATS:  gEditableMapObject.SetFlats(v); break;
  case Metadata::FMD_BUILDING_LEVELS: gEditableMapObject.SetBuildingLevels(v); break;
  case Metadata::FMD_TURN_LANES:
  case Metadata::FMD_TURN_LANES_FORWARD:
  case Metadata::FMD_TURN_LANES_BACKWARD:
  case Metadata::FMD_MAXSPEED:
  case Metadata::FMD_HEIGHT:
  case Metadata::FMD_MIN_HEIGHT:
  case Metadata::FMD_DENOMINATION:
  case Metadata::FMD_TEST_ID:
  case Metadata::FMD_COUNT:
    break;
  }
}

JNIEXPORT jboolean JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeSaveEditedFeature(JNIEnv *, jclass)
{
  switch (g_framework->NativeFramework()->SaveEditedMapObject(gEditableMapObject))
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
  auto const & editable = gEditableMapObject.GetEditableFields();
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
  return gEditableMapObject.IsAddressEditable();
}

JNIEXPORT jboolean JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeIsNameEditable(JNIEnv * env, jclass clazz)
{
  return gEditableMapObject.IsNameEditable();
}

JNIEXPORT jstring JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeGetDefaultName(JNIEnv * env, jclass)
{
  // TODO(yunikkk): add multilanguage names support via EditableMapObject::GetLocalizedName().
  return jni::ToJavaString(env, gEditableMapObject.GetDefaultName());
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeSetDefaultName(JNIEnv * env, jclass, jstring name)
{
  StringUtf8Multilang names = gEditableMapObject.GetName();
  // TODO(yunikkk): add multilanguage names support.
  names.AddString(StringUtf8Multilang::kDefaultCode, jni::ToNativeString(env, name));
  gEditableMapObject.SetName(names);
}

JNIEXPORT jstring JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeGetStreet(JNIEnv * env, jclass)
{
  return jni::ToJavaString(env, gEditableMapObject.GetStreet());
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeSetStreet(JNIEnv * env, jclass, jstring street)
{
  gEditableMapObject.SetStreet(jni::ToNativeString(env, street));
}

JNIEXPORT jstring JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeGetHouseNumber(JNIEnv * env, jclass)
{
  return jni::ToJavaString(env, gEditableMapObject.GetHouseNumber());
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeSetHouseNumber(JNIEnv * env, jclass, jstring houseNumber)
{
  gEditableMapObject.SetHouseNumber(jni::ToNativeString(env, houseNumber));
}

JNIEXPORT jobjectArray JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeGetNearbyStreets(JNIEnv * env, jclass clazz)
{
  auto const & streets = gEditableMapObject.GetNearbyStreets();
  int const size = streets.size();
  jobjectArray jStreets = env->NewObjectArray(size, jni::GetStringClass(env), 0);
  for (int i = 0; i < size; ++i)
    env->SetObjectArrayElement(jStreets, i, jni::TScopedLocalRef(env, jni::ToJavaString(env, streets[i])).get());
  return jStreets;
}

JNIEXPORT jboolean JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeHasWifi(JNIEnv *, jclass)
{
  // TODO(AlexZ): Support 3-state: yes, no, unknown.
  return gEditableMapObject.GetMetadata().Get(feature::Metadata::FMD_INTERNET) == "wlan";
}


JNIEXPORT jboolean JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeHasSomethingToUpload(JNIEnv * env, jclass clazz)
{
  return Editor::Instance().HaveSomethingToUpload();
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeUploadChanges(JNIEnv * env, jclass clazz, jstring token, jstring secret)
{
  Editor::Instance().UploadChanges(jni::ToNativeString(env, token),
                                   jni::ToNativeString(env, secret),
                                   {{"version", "TODO android"}}, nullptr);
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
  CHECK(frm->GetEditableMapObject(info.GetID(), gEditableMapObject), ("Invalid feature in the place page."));
 }
} // extern "C"

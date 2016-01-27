#include <jni.h>

#include "com/mapswithme/core/jni_helper.hpp"
#include "com/mapswithme/maps/Framework.hpp"

#include "base/logging.hpp"
#include "indexer/osm_editor.hpp"

#include "std/algorithm.hpp"
#include "std/set.hpp"
#include "std/vector.hpp"

namespace
{
using feature::Metadata;
using osm::Editor;

FeatureType * activeFeature()
{
  return g_framework->GetActiveUserMark()->GetFeature();
}
} // namespace

extern "C"
{
using osm::Editor;

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeSetMetadata(JNIEnv * env, jclass clazz, jint type, jstring value)
{
  auto * feature = activeFeature();
  CHECK(feature, ("Feature is not editable!"));
  auto & metadata = feature->GetMetadata();
  metadata.Set(static_cast<Metadata::EType>(type), jni::ToNativeString(env, value));
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeEditFeature(JNIEnv * env, jclass clazz, jstring street, jstring houseNumber)
{
  auto * feature = activeFeature();
  CHECK(feature, ("Feature is not editable!"));
  Editor::Instance().EditFeature(*feature, jni::ToNativeString(env, street), jni::ToNativeString(env, houseNumber));
}

JNIEXPORT jintArray JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeGetEditableMetadata(JNIEnv * env, jclass clazz)
{
  auto const * feature = activeFeature();
  auto const & editableTypes = feature ? Editor::Instance().EditableMetadataForType(*feature)
                                       : vector<Metadata::EType>{};
  int const size = editableTypes.size();
  jintArray jEditableTypes = env->NewIntArray(size);
  jint * arr = env->GetIntArrayElements(jEditableTypes, 0);
  for (int i = 0; i < size; i++)
    arr[i] = static_cast<jint>(editableTypes[i]);
  env->ReleaseIntArrayElements(jEditableTypes, arr, 0);

  return jEditableTypes;
}

JNIEXPORT jboolean JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeIsAddressEditable(JNIEnv * env, jclass clazz)
{
  auto const * feature = activeFeature();
  return feature && Editor::Instance().IsAddressEditable(*feature);
}

JNIEXPORT jboolean JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeIsNameEditable(JNIEnv * env, jclass clazz)
{
  auto const * feature = activeFeature();
  return feature && Editor::Instance().IsNameEditable(*feature);
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeSetName(JNIEnv * env, jclass clazz, jstring name)
{
  auto * feature = activeFeature();
  CHECK(feature, ("Feature is not editable!"));
  auto names = feature->GetNames();
  names.AddString(StringUtf8Multilang::DEFAULT_CODE, jni::ToNativeString(env, name));
  feature->SetNames(names);
}

JNIEXPORT jobjectArray JNICALL
Java_com_mapswithme_maps_editor_Editor_nativeGetNearbyStreets(JNIEnv * env, jclass clazz)
{
  auto const * feature = activeFeature();
  auto const & streets = feature ? g_framework->NativeFramework()->GetNearbyFeatureStreets(*feature)
                                 : vector<string>{};
  int const size = streets.size();
  jobjectArray jStreets = env->NewObjectArray(size, jni::GetStringClass(env), 0);
  for (int i = 0; i < size; i++)
    env->SetObjectArrayElement(jStreets, i, jni::TScopedLocalRef(env, jni::ToJavaString(env, streets[i])).get());
  return jStreets;
}
} // extern "C"

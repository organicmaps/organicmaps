#pragma once

#include <jni.h>

#include "com/mapswithme/core/jni_helper.hpp"
#include "com/mapswithme/maps/Framework.hpp"

#include "map/user_mark.hpp"

namespace usermark_helper
{
// should be equal with definitions in MapObject.java
static constexpr int kPoi = 0;
static constexpr int kApiPoint = 1;
static constexpr int kBookmark = 2;
static constexpr int kMyPosition = 3;
static constexpr int kSearch = 4;

// Fills mapobject's metadata from UserMark
void InjectMetadata(JNIEnv * env, jclass clazz, jobject const mapObject, feature::Metadata const & metadata);

template <class T>
T const * CastMark(UserMark const * data);

pair<jintArray, jobjectArray> NativeMetadataToJavaMetadata(JNIEnv * env, feature::Metadata const & metadata);

void FillAddressAndMetadata(UserMark const * mark, search::AddressInfo & info, feature::Metadata & metadata);

jobject CreateBookmark(int categoryId, int bookmarkId, string const & typeName, feature::Metadata const & metadata);

jobject CreateMapObject(int mapObjectType, string const & name, double lat, double lon, string const & typeName, feature::Metadata const & metadata);

jobject CreateMapObject(UserMark const * userMark);
} // namespace usermark

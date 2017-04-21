#pragma once

#include <jni.h>

#include "com/mapswithme/core/jni_helper.hpp"
#include "com/mapswithme/maps/Framework.hpp"

namespace place_page
{
class Info;
}  // namespace place_page

// TODO(yunikkk): this helper is redundant with new place page info approach.
// It's better to refactor MapObject in Java, may be removing it at all, and to make simple jni getters for
// globally stored place_page::Info object. Code should be clean and easy to support in this way.
namespace usermark_helper
{
// TODO(yunikkk): PP can be POI and bookmark at the same time. And can be even POI + bookmark + API at the same time.
// The same for search result: it can be also a POI and bookmark (and API!).
// That is one of the reasons why existing solution should be refactored.
// should be equal with definitions in MapObject.java
static constexpr int kPoi = 0;
static constexpr int kApiPoint = 1;
static constexpr int kBookmark = 2;
static constexpr int kMyPosition = 3;
static constexpr int kSearch = 4;

// Fills mapobject's metadata.
void InjectMetadata(JNIEnv * env, jclass clazz, jobject const mapObject, feature::Metadata const & metadata);

jobject CreateMapObject(JNIEnv * env, int mapObjectType, string const & title, string const & subtitle,
                        double lat, double lon, feature::Metadata const & metadata);

jobject CreateMapObject(JNIEnv * env, place_page::Info const & info);

jobjectArray ToBannersArray(JNIEnv * env, vector<ads::Banner> const & banners);

jobject CreateLocalAdInfo(JNIEnv * env, place_page::Info const & info);
}  // namespace usermark_helper

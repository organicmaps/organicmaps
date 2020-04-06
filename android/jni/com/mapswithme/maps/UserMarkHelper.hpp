#pragma once

#include <jni.h>

#include "com/mapswithme/core/jni_helper.hpp"
#include "com/mapswithme/maps/Framework.hpp"

#include <vector>

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

static constexpr int kPriceRateUndefined = -1;

// Fills mapobject's metadata.
void InjectMetadata(JNIEnv * env, jclass clazz, jobject const mapObject, feature::Metadata const & metadata);

jobject CreateMapObject(JNIEnv * env, place_page::Info const & info);

jobject CreateElevationInfo(JNIEnv * env, std::string const & serverId, ElevationInfo const & info);

jobjectArray ToBannersArray(JNIEnv * env, std::vector<ads::Banner> const & banners);

jobjectArray ToRatingArray(JNIEnv * env, std::vector<std::string> const & ratingCategories);

jintArray ToReachableByTaxiProvidersArray(JNIEnv * env,
                                          std::vector<taxi::Provider::Type> const & types);

jobject CreateLocalAdInfo(JNIEnv * env, place_page::Info const & info);

jobject CreateRoutePointInfo(JNIEnv * env, place_page::Info const & info);

jobject CreateFeatureId(JNIEnv * env, FeatureID const & fid);
jobjectArray ToFeatureIdArray(JNIEnv * env, std::vector<FeatureID> const & ids);
}  // namespace usermark_helper

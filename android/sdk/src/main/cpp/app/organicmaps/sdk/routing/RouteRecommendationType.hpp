#pragma once

#include "app/organicmaps/sdk/core/jni_helper.hpp"

jobject GetRebuildAfterPointsLoading(JNIEnv * env)
{
  static jobject rebuildAfterPointsLoading = nullptr;
  if (rebuildAfterPointsLoading)
    return rebuildAfterPointsLoading;

  // Find the RouteRecommendationType class
  jclass routeRecommendationTypeClass = env->FindClass("app/organicmaps/sdk/routing/RouteRecommendationType");
  ASSERT(routeRecommendationTypeClass, ());

  // Get the values() method of RouteRecommendationType
  jmethodID valuesMethod = env->GetStaticMethodID(routeRecommendationTypeClass, "values",
                                                  "()[Lapp/organicmaps/sdk/routing/RouteRecommendationType;");
  ASSERT(valuesMethod, ());

  // Call values() to get all enum constants
  jobjectArray enumConstants = (jobjectArray)env->CallStaticObjectMethod(routeRecommendationTypeClass, valuesMethod);
  ASSERT(enumConstants, ());

  // Retrieve the first (and only) constant, RebuildAfterPointsLoading
  rebuildAfterPointsLoading = env->NewGlobalRef(env->GetObjectArrayElement(enumConstants, 0));
  ASSERT(rebuildAfterPointsLoading, ());

  return rebuildAfterPointsLoading;
}

jobject GetRouteRecommendationType(JNIEnv * env, RoutingManager::Recommendation recommendation)
{
  static jobject rebuildAfterPointsLoading = nullptr;
  static jobject hasAlprs = nullptr;

  jclass routeRecommendationTypeClass = env->FindClass("app/organicmaps/sdk/routing/RouteRecommendationType");
  ASSERT(routeRecommendationTypeClass, ());

  jmethodID valuesMethod = env->GetStaticMethodID(routeRecommendationTypeClass, "values",
                                                  "()[Lapp/organicmaps/sdk/routing/RouteRecommendationType;");
  ASSERT(valuesMethod, ());

  jobjectArray enumConstants = (jobjectArray)env->CallStaticObjectMethod(routeRecommendationTypeClass, valuesMethod);
  ASSERT(enumConstants, ());

  if (recommendation == RoutingManager::Recommendation::RebuildAfterPointsLoading)
  {
    if (!rebuildAfterPointsLoading)
      rebuildAfterPointsLoading = env->NewGlobalRef(env->GetObjectArrayElement(enumConstants, 0));
    return rebuildAfterPointsLoading;
  }
  else if (recommendation == RoutingManager::Recommendation::HasAlprs)
  {
    if (!hasAlprs)
      hasAlprs = env->NewGlobalRef(env->GetObjectArrayElement(enumConstants, 1));
    return hasAlprs;
  }

  ASSERT_FAIL("Unknown recommendation type");
}

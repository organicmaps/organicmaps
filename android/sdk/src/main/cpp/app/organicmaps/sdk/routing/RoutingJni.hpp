#pragma once

#include <jni.h>

// Pulls in RouteMarkData, RouteMarkType, TransitRouteInfo and routing::FollowingInfo.
#include "map/routing_manager.hpp"

#include "geometry/point_with_altitude.hpp"

#include <vector>

namespace place_page
{
class Info;
}  // namespace place_page

namespace routing_jni
{
jobject CreateRoutingInfo(JNIEnv * env, routing::FollowingInfo const & info, RoutingManager & rm);
jobject GetRouteRecommendationType(JNIEnv * env, RoutingManager::Recommendation recommendation);
jobject CreateTransitRouteInfo(JNIEnv * env, TransitRouteInfo const & routeInfo);
jobjectArray CreateJunctionInfoArray(JNIEnv * env, std::vector<geometry::PointWithAltitude> const & junctionPoints);

RouteMarkType GetRouteMarkType(JNIEnv * env, jobject markType);
jobject CreateRoutePointInfo(JNIEnv * env, place_page::Info const & info);
jobjectArray CreateRouteMarkDataArray(JNIEnv * env, std::vector<RouteMarkData> const & points);
}  // namespace routing_jni

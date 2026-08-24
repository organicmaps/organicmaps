#include "app/organicmaps/sdk/routing/RoutingJni.hpp"

#include "app/organicmaps/sdk/core/jni_helper.hpp"
#include "app/organicmaps/sdk/util/Distance.hpp"

#include "map/place_page_info.hpp"

#include "routing/lanes/lane_info.hpp"
#include "routing/routing_options.hpp"
#include "routing/turns.hpp"

#include "indexer/road_shields_parser.hpp"

#include "geometry/mercator.hpp"

#include "base/assert.hpp"

#include <string>

namespace routing_jni
{
namespace
{
// Every routing enum below is mirrored one to one by a Java enum, so the constant is looked up by name.
// jni::GetStaticFieldID catches a name drift between the two in Debug; a raw env->GetStaticFieldID
// would silently return null and leave a pending NoSuchFieldError for the next JNI call to trip over.
jobject ToJavaEnum(JNIEnv * env, jclass clazz, char const * signature, char const * name)
{
  return env->GetStaticObjectField(clazz, jni::GetStaticFieldID(env, clazz, name, signature));
}

// Java spells the absence of a turn NoTurn, DebugPrint spells it None; everything else matches.
template <typename Direction>
jobject ToJavaDirection(JNIEnv * env, jclass clazz, char const * signature, Direction direction)
{
  auto const name = direction == Direction::None ? std::string("NoTurn") : DebugPrint(direction);
  return ToJavaEnum(env, clazz, signature, name.c_str());
}

jobject ToJavaCarDirection(JNIEnv * env, routing::turns::CarDirection carDirection)
{
  static jclass const clazz = jni::GetGlobalClassRef(env, "app/organicmaps/sdk/routing/CarDirection");
  return ToJavaDirection(env, clazz, "Lapp/organicmaps/sdk/routing/CarDirection;", carDirection);
}

jobject ToJavaPedestrianDirection(JNIEnv * env, routing::turns::PedestrianDirection pedestrianDirection)
{
  static jclass const clazz = jni::GetGlobalClassRef(env, "app/organicmaps/sdk/routing/PedestrianDirection");
  return ToJavaDirection(env, clazz, "Lapp/organicmaps/sdk/routing/PedestrianDirection;", pedestrianDirection);
}

jobject ToJavaRoundaboutDirection(JNIEnv * env, routing::turns::RoundaboutDirection direction)
{
  static jclass const clazz = jni::GetGlobalClassRef(env, "app/organicmaps/sdk/routing/RoundaboutDirection");
  return ToJavaEnum(env, clazz, "Lapp/organicmaps/sdk/routing/RoundaboutDirection;", DebugPrint(direction).c_str());
}

jobject ToJavaLaneWay(JNIEnv * env, routing::turns::lanes::LaneWay const & laneWay)
{
  static jclass const clazz = jni::GetGlobalClassRef(env, "app/organicmaps/sdk/routing/LaneWay");
  return ToJavaEnum(env, clazz, "Lapp/organicmaps/sdk/routing/LaneWay;", DebugPrint(laneWay).c_str());
}

jobjectArray CreateLanesInfo(JNIEnv * env, routing::turns::lanes::LanesInfo const & lanes)
{
  using namespace jni;

  if (lanes.empty())
    return nullptr;

  static jclass const laneWayClass = GetGlobalClassRef(env, "app/organicmaps/sdk/routing/LaneWay");
  static jclass const laneInfoClass = GetGlobalClassRef(env, "app/organicmaps/sdk/routing/LaneInfo");
  auto const lanesSize = static_cast<jsize>(lanes.size());
  jobjectArray jLanes = env->NewObjectArray(lanesSize, laneInfoClass, nullptr);
  ASSERT(jLanes, (DescribeException()));
  // Java signature : LaneInfo(LaneWay[] laneWays, LaneWay activeLane)
  static jmethodID const ctorLaneInfoID = GetConstructorID(
      env, laneInfoClass, "([Lapp/organicmaps/sdk/routing/LaneWay;Lapp/organicmaps/sdk/routing/LaneWay;)V");

  for (jsize j = 0; j < lanesSize; ++j)
  {
    auto const laneWays = lanes[j].laneWays.GetActiveLaneWays();
    auto const laneWaysSize = static_cast<jsize>(laneWays.size());
    TScopedLocalObjectArrayRef jLaneWays(env, env->NewObjectArray(laneWaysSize, laneWayClass, nullptr));
    ASSERT(jLaneWays.get(), (DescribeException()));
    for (jsize i = 0; i < laneWaysSize; ++i)
    {
      TScopedLocalRef jLaneWay(env, ToJavaLaneWay(env, laneWays[i]));
      env->SetObjectArrayElement(jLaneWays.get(), i, jLaneWay.get());
    }
    TScopedLocalRef jLaneInfo(env, env->NewObject(laneInfoClass, ctorLaneInfoID, jLaneWays.get(),
                                                  ToJavaLaneWay(env, lanes[j].recommendedWay)));
    ASSERT(jLaneInfo.get(), (DescribeException()));
    env->SetObjectArrayElement(jLanes, j, jLaneInfo.get());
  }

  return jLanes;
}

char const * ToJavaRoadShieldTypeName(ftypes::RoadShieldType roadShieldType)
{
  switch (roadShieldType)
  {
    using enum ftypes::RoadShieldType;
  case Default: [[fallthrough]];
  case Hidden: [[fallthrough]];
  case Generic_White: return "GenericWhite";
  case Generic_Green: return "GenericGreen";
  case Generic_Blue: return "GenericBlue";
  case Generic_Red: return "GenericRed";
  case Generic_Orange: return "GenericOrange";
  case US_Interstate: return "USInterstate";
  case US_Highway: return "USHighway";
  case UK_Highway: return "UKHighway";
  default: UNREACHABLE();
  }
}

jobject ToJavaRoadShieldType(JNIEnv * env, ftypes::RoadShieldType roadShieldType)
{
  static jclass const clazz = jni::GetGlobalClassRef(env, "app/organicmaps/sdk/routing/roadshield/RoadShieldType");
  return ToJavaEnum(env, clazz, "Lapp/organicmaps/sdk/routing/roadshield/RoadShieldType;",
                    ToJavaRoadShieldTypeName(roadShieldType));
}

jobject ToJavaRoadShield(JNIEnv * env, ftypes::RoadShield const & roadShield)
{
  using namespace jni;

  static jclass const klass = GetGlobalClassRef(env, "app/organicmaps/sdk/routing/roadshield/RoadShield");
  // clang-format off
  static jmethodID const ctorRouteInfoID = GetConstructorID(env, klass,
    "("
    "Lapp/organicmaps/sdk/routing/roadshield/RoadShieldType;"  // type
    "Ljava/lang/String;"                                       // text
    "Ljava/lang/String;"                                       // additionalText
    ")V"
  );
  // clang-format on

  // clang-format off
  jobject const result = env->NewObject(klass, ctorRouteInfoID,
    ToJavaRoadShieldType(env, roadShield.m_type),
    ToJavaString(env, roadShield.m_name),
    ToJavaString(env, roadShield.m_additionalText)
  );
  // clang-format on
  ASSERT(result, (DescribeException()));
  return result;
}

jobjectArray ToJavaRoadShieldsArray(JNIEnv * env, ftypes::RoadShieldsSetT const & roadShields)
{
  static jclass const roadShieldClass =
      jni::GetGlobalClassRef(env, "app/organicmaps/sdk/routing/roadshield/RoadShield");
  return jni::ToJavaArray(env, roadShieldClass, roadShields, ToJavaRoadShield);
}

jobject ToJavaRoadShieldInfo(JNIEnv * env, routing::FollowingInfo::RoadShieldInfo const & roadShieldInfo)
{
  using namespace jni;

  if (roadShieldInfo.m_targetRoadShields.empty() &&
      roadShieldInfo.m_junctionInfoPosition.first == roadShieldInfo.m_junctionInfoPosition.second)
    return nullptr;

  static jclass const klass = GetGlobalClassRef(env, "app/organicmaps/sdk/routing/roadshield/RoadShieldInfo");
  // clang-format off
  static jmethodID const ctorRouteInfoID = GetConstructorID(env, klass,
    "("
    "[Lapp/organicmaps/sdk/routing/roadshield/RoadShield;"  // targetRoadShields
    "I"                                                     // targetRoadShieldsIndexStart
    "I"                                                     // targetRoadShieldsIndexEnd
    "I"                                                     // junctionInfoIndexStart
    "I"                                                     // junctionInfoIndexEnd
    ")V"
  );
  // clang-format on

  // clang-format off
  jobject const result = env->NewObject(klass, ctorRouteInfoID,
    roadShieldInfo.m_targetRoadShields.empty() ? nullptr : ToJavaRoadShieldsArray(env, roadShieldInfo.m_targetRoadShields),
    roadShieldInfo.m_targetRoadShieldsPosition.first,
    roadShieldInfo.m_targetRoadShieldsPosition.second,
    roadShieldInfo.m_junctionInfoPosition.first,
    roadShieldInfo.m_junctionInfoPosition.second
  );
  // clang-format on
  ASSERT(result, (DescribeException()));
  return result;
}

jobjectArray CreateTransitStepInfoArray(JNIEnv * env, std::vector<TransitStepInfo> const & steps)
{
  using namespace jni;

  static jclass const transitStepClass = GetGlobalClassRef(env, "app/organicmaps/sdk/routing/TransitStepInfo");
  // Java signature : TransitStepInfo(int type, @Nullable String distance, @Nullable String distanceUnits,
  //                                  int timeInSec, @Nullable String number, int color, int intermediateIndex)
  static jmethodID const transitStepConstructor =
      GetConstructorID(env, transitStepClass, "(ILjava/lang/String;Ljava/lang/String;ILjava/lang/String;II)V");

  return ToJavaArray(env, transitStepClass, steps, [](JNIEnv * env, TransitStepInfo const & stepInfo)
  {
    TScopedLocalRef const distance(env, ToJavaString(env, stepInfo.m_distanceStr));
    TScopedLocalRef const distanceUnits(env, ToJavaString(env, stepInfo.m_distanceUnitsSuffix));
    TScopedLocalRef const number(env, ToJavaString(env, stepInfo.m_number));
    return env->NewObject(transitStepClass, transitStepConstructor, static_cast<jint>(stepInfo.m_type), distance.get(),
                          distanceUnits.get(), static_cast<jint>(stepInfo.m_timeInSec), number.get(),
                          static_cast<jint>(stepInfo.m_colorARGB), static_cast<jint>(stepInfo.m_intermediateIndex));
  });
}

routing::RoutingOptions::Road ToRoutingOptionsRoad(jint option)
{
  using routing::RoutingOptions;

  auto const road = static_cast<uint8_t>(1u << static_cast<int>(option));
  CHECK_LESS(road, static_cast<uint8_t>(RoutingOptions::Road::Max), ());
  return static_cast<RoutingOptions::Road>(road);
}
}  // namespace

jobject CreateRoutingInfo(JNIEnv * env, routing::FollowingInfo const & info, RoutingManager & rm)
{
  using namespace jni;

  static jclass const klass = GetGlobalClassRef(env, "app/organicmaps/sdk/routing/RoutingInfo");
  // clang-format off
  static jmethodID const ctorRouteInfoID = GetConstructorID(env, klass,
    "("
    "Lapp/organicmaps/sdk/util/Distance;"                      // distToTarget
    "Lapp/organicmaps/sdk/util/Distance;"                      // distToTurn
    "Ljava/lang/String;"                                       // currentStreet
    "Ljava/lang/String;"                                       // nextStreet
    "Lapp/organicmaps/sdk/routing/roadshield/RoadShieldInfo;"  // nextStreetRoadShields
    "Ljava/lang/String;"                                       // nextNextStreet
    "Lapp/organicmaps/sdk/routing/roadshield/RoadShieldInfo;"  // nextNextStreetRoadShields
    "D"                                                        // completionPercent
    "Lapp/organicmaps/sdk/routing/CarDirection;"               // carTurnDirection
    "Lapp/organicmaps/sdk/routing/CarDirection;"               // carNextTurnDirection
    "Lapp/organicmaps/sdk/routing/PedestrianDirection;"        // pedestrianDirection
    "I"                                                        // exitNum
    "I"                                                        // exitAngle
    "Lapp/organicmaps/sdk/routing/RoundaboutDirection;"        // roundaboutDirection
    "Z"                                                        // hasRoundaboutExit
    "I"                                                        // nextExitNum
    "I"                                                        // nextExitAngle
    "Lapp/organicmaps/sdk/routing/RoundaboutDirection;"        // nextRoundaboutDirection
    "Z"                                                        // nextHasRoundaboutExit
    "I"                                                        // totalTime
    "[Lapp/organicmaps/sdk/routing/LaneInfo;"                  // lanes
    "D"                                                        // speedLimitMps
    "Z"                                                        // speedLimitExceeded
    "Z"                                                        // shouldPlayWarningSignal
    ")V"
  );
  // clang-format on

  // clang-format off
  jobject const result = env->NewObject(klass, ctorRouteInfoID,
    ToJavaDistance(env, info.m_distToTarget),
    ToJavaDistance(env, info.m_distToTurn),
    ToJavaString(env, info.m_currentStreetName),
    ToJavaString(env, info.m_nextStreetName),
    ToJavaRoadShieldInfo(env, info.m_nextStreetShields),
    ToJavaString(env, info.m_nextNextStreetName),
    ToJavaRoadShieldInfo(env, info.m_nextNextStreetShields),
    info.m_completionPercent,
    ToJavaCarDirection(env, info.m_turn),
    ToJavaCarDirection(env, info.m_nextTurn),
    ToJavaPedestrianDirection(env, info.m_pedestrianTurn),
    info.m_exitNum,
    info.m_roundaboutInfo.m_exitAngle,
    ToJavaRoundaboutDirection(env, info.m_roundaboutInfo.m_direction),
    static_cast<jboolean>(info.m_roundaboutInfo.m_hasExit),
    info.m_nextExitNum,
    info.m_nextRoundaboutInfo.m_exitAngle,
    ToJavaRoundaboutDirection(env, info.m_nextRoundaboutInfo.m_direction),
    static_cast<jboolean>(info.m_nextRoundaboutInfo.m_hasExit),
    info.m_time,
    CreateLanesInfo(env, info.m_lanes),
    info.m_speedLimitMps,
    static_cast<jboolean>(rm.IsSpeedCamLimitExceeded()),
    static_cast<jboolean>(rm.GetSpeedCamManager().ShouldPlayBeepSignal())
  );
  // clang-format on
  ASSERT(result, (DescribeException()));
  return result;
}

jobject GetRouteRecommendationType(JNIEnv * env, RoutingManager::Recommendation recommendation)
{
  static jclass const clazz = jni::GetGlobalClassRef(env, "app/organicmaps/sdk/routing/RouteRecommendationType");
  switch (recommendation)
  {
  case RoutingManager::Recommendation::RebuildAfterPointsLoading:
  {
    static jfieldID const fieldId = jni::GetStaticFieldID(env, clazz, "RebuildAfterPointsLoading",
                                                          "Lapp/organicmaps/sdk/routing/RouteRecommendationType;");
    return env->GetStaticObjectField(clazz, fieldId);
  }
  }
  UNREACHABLE();
}

jobject CreateTransitRouteInfo(JNIEnv * env, TransitRouteInfo const & routeInfo)
{
  using namespace jni;

  TScopedLocalObjectArrayRef const steps(env, CreateTransitStepInfoArray(env, routeInfo.m_steps));

  static jclass const transitRouteInfoClass = GetGlobalClassRef(env, "app/organicmaps/sdk/routing/TransitRouteInfo");
  // Java signature : TransitRouteInfo(@NonNull String totalDistance, @NonNull String totalDistanceUnits,
  //                                   int totalTimeInSec, @NonNull String totalPedestrianDistance, @NonNull String
  //                                   totalPedestrianDistanceUnits, int totalPedestrianTimeInSec, @NonNull
  //                                   TransitStepInfo[] steps)
  static jmethodID const transitRouteInfoConstructor =
      GetConstructorID(env, transitRouteInfoClass,
                       "(Ljava/lang/String;Ljava/lang/String;I"
                       "Ljava/lang/String;Ljava/lang/String;I"
                       "[Lapp/organicmaps/sdk/routing/TransitStepInfo;)V");
  TScopedLocalRef const distance(env, ToJavaString(env, routeInfo.m_totalDistanceStr));
  TScopedLocalRef const distanceUnits(env, ToJavaString(env, routeInfo.m_totalDistanceUnitsSuffix));
  TScopedLocalRef const distancePedestrian(env, ToJavaString(env, routeInfo.m_totalPedestrianDistanceStr));
  TScopedLocalRef const distancePedestrianUnits(env, ToJavaString(env, routeInfo.m_totalPedestrianUnitsSuffix));
  return env->NewObject(transitRouteInfoClass, transitRouteInfoConstructor, distance.get(), distanceUnits.get(),
                        static_cast<jint>(routeInfo.m_totalTimeInSec), distancePedestrian.get(),
                        distancePedestrianUnits.get(), static_cast<jint>(routeInfo.m_totalPedestrianTimeInSec),
                        steps.get());
}

jobjectArray CreateJunctionInfoArray(JNIEnv * env, std::vector<geometry::PointWithAltitude> const & junctionPoints)
{
  using namespace jni;

  static jclass const junctionClazz = GetGlobalClassRef(env, "app/organicmaps/sdk/routing/JunctionInfo");
  // Java signature : JunctionInfo(double lat, double lon)
  static jmethodID const junctionConstructor = GetConstructorID(env, junctionClazz, "(DD)V");

  return ToJavaArray(env, junctionClazz, junctionPoints,
                     [](JNIEnv * env, geometry::PointWithAltitude const & pointWithAltitude)
  {
    auto const ll = pointWithAltitude.ToLatLon();
    return env->NewObject(junctionClazz, junctionConstructor, ll.m_lat, ll.m_lon);
  });
}

RouteMarkType GetRouteMarkType(JNIEnv * env, jobject markType)
{
  static jmethodID const ordinal = jni::GetMethodID(env, markType, "ordinal", "()I");

  return static_cast<RouteMarkType>(env->CallIntMethod(markType, ordinal));
}

jobject CreateRoutePointInfo(JNIEnv * env, place_page::Info const & info)
{
  static jclass const clazz = jni::GetGlobalClassRef(env, "app/organicmaps/sdk/routing/RoutePointInfo");
  static jmethodID const ctorId = jni::GetConstructorID(env, clazz, "(II)V");
  return env->NewObject(clazz, ctorId, static_cast<jint>(info.GetRouteMarkType()), info.GetIntermediateIndex());
}

jobjectArray CreateRouteMarkDataArray(JNIEnv * env, std::vector<RouteMarkData> const & points)
{
  using namespace jni;

  static jclass const pointClazz = GetGlobalClassRef(env, "app/organicmaps/sdk/routing/RouteMarkData");
  // Java signature : RouteMarkData(String title, String subtitle, int pointType,
  //                                int intermediateIndex, boolean isVisible, boolean isMyPosition,
  //                                boolean isPassed, double lat, double lon)
  static jmethodID const pointConstructor =
      GetConstructorID(env, pointClazz, "(Ljava/lang/String;Ljava/lang/String;IIZZZDD)V");
  return ToJavaArray(env, pointClazz, points, [](JNIEnv * env, RouteMarkData const & data)
  {
    TScopedLocalRef const title(env, ToJavaString(env, data.m_title));
    TScopedLocalRef const subtitle(env, ToJavaString(env, data.m_subTitle));
    return env->NewObject(pointClazz, pointConstructor, title.get(), subtitle.get(),
                          static_cast<jint>(data.m_pointType), static_cast<jint>(data.m_intermediateIndex),
                          static_cast<jboolean>(data.m_isVisible), static_cast<jboolean>(data.m_isMyPosition),
                          static_cast<jboolean>(data.m_isPassed), mercator::YToLat(data.m_position.y),
                          mercator::XToLon(data.m_position.x));
  });
}
}  // namespace routing_jni

extern "C"
{
JNIEXPORT jboolean Java_app_organicmaps_sdk_routing_RoutingOptions_nativeHasOption(JNIEnv *, jclass, jint option)
{
  using routing::RoutingOptions;

  RoutingOptions const routingOptions = RoutingOptions::LoadCarOptionsFromSettings();
  return static_cast<jboolean>(routingOptions.Has(routing_jni::ToRoutingOptionsRoad(option)));
}

JNIEXPORT void Java_app_organicmaps_sdk_routing_RoutingOptions_nativeAddOption(JNIEnv *, jclass, jint option)
{
  using routing::RoutingOptions;

  RoutingOptions routingOptions = RoutingOptions::LoadCarOptionsFromSettings();
  routingOptions.Add(routing_jni::ToRoutingOptionsRoad(option));
  RoutingOptions::SaveCarOptionsToSettings(routingOptions);
}

JNIEXPORT void Java_app_organicmaps_sdk_routing_RoutingOptions_nativeRemoveOption(JNIEnv *, jclass, jint option)
{
  using routing::RoutingOptions;

  RoutingOptions routingOptions = RoutingOptions::LoadCarOptionsFromSettings();
  routingOptions.Remove(routing_jni::ToRoutingOptionsRoad(option));
  RoutingOptions::SaveCarOptionsToSettings(routingOptions);
}
}

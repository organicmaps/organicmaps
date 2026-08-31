#include "map/user_mark.hpp"

#include "app/organicmaps/sdk/Framework.hpp"
#include "app/organicmaps/sdk/core/jni_helper.hpp"

extern "C"
{
JNIEXPORT void Java_app_organicmaps_sdk_api_ApiController_nativeClearApiPoints(JNIEnv * env, jclass clazz)
{
  frm()->GetBookmarkManager().GetEditSession().ClearGroup(UserMark::Type::API);
}

JNIEXPORT jint Java_app_organicmaps_sdk_api_ApiController_nativeParseAndSetApiUrl(JNIEnv * env, jclass clazz,
                                                                                  jstring url)
{
  return static_cast<jint>(frm()->ParseAndSetApiURL(jni::ToNativeString(env, url)));
}

JNIEXPORT jobject Java_app_organicmaps_sdk_api_ApiController_nativeGetParsedRoutingData(JNIEnv * env, jclass clazz)
{
  using namespace url_scheme;
  static jclass const pointClazz = jni::GetGlobalClassRef(env, "app/organicmaps/sdk/api/RoutePoint");
  // Java signature : RoutePoint(double lat, double lon, String name)
  static jmethodID const pointConstructor = jni::GetConstructorID(env, pointClazz, "(DDLjava/lang/String;)V");

  static jclass const routeDataClazz = jni::GetGlobalClassRef(env, "app/organicmaps/sdk/api/ParsedRoutingData");
  // Java signature : ParsedRoutingData(RoutePoint[] points, int routerType) {
  static jmethodID const routeDataConstructor =
      jni::GetConstructorID(env, routeDataClazz, "([Lapp/organicmaps/sdk/api/RoutePoint;I)V");

  auto const & routingData = frm()->GetParsedRoutingData();
  jobjectArray points =
      jni::ToJavaArray(env, pointClazz, routingData.m_points, [](JNIEnv * env, RoutePoint const & point)
  {
    jni::TScopedLocalRef const name(env, jni::ToJavaString(env, point.m_name));
    return env->NewObject(pointClazz, pointConstructor, mercator::YToLat(point.m_org.y),
                          mercator::XToLon(point.m_org.x), name.get());
  });

  return env->NewObject(routeDataClazz, routeDataConstructor, points, routingData.m_type);
}

JNIEXPORT jobject Java_app_organicmaps_sdk_api_ApiController_nativeGetParsedSearchRequest(JNIEnv * env, jclass clazz)
{
  using namespace url_scheme;
  static jclass const cl = jni::GetGlobalClassRef(env, "app/organicmaps/sdk/api/ParsedSearchRequest");
  // Java signature : ParsedSearchRequest(String query, String locale, double lat, double lon, boolean isSearchOnMap)
  static jmethodID const ctor = jni::GetConstructorID(env, cl, "(Ljava/lang/String;Ljava/lang/String;DDZ)V");
  auto const & r = frm()->GetParsedSearchRequest();
  ms::LatLon const center = frm()->GetParsedCenterLatLon();
  return env->NewObject(cl, ctor, jni::ToJavaString(env, r.m_query), jni::ToJavaString(env, r.m_locale), center.m_lat,
                        center.m_lon, r.m_isSearchOnMap);
}

JNIEXPORT jstring Java_app_organicmaps_sdk_api_ApiController_nativeGetParsedAppName(JNIEnv * env, jclass)
{
  std::string const & appName = frm()->GetParsedAppName();
  return (appName.empty()) ? nullptr : jni::ToJavaString(env, appName);
}

JNIEXPORT jstring Java_app_organicmaps_sdk_api_ApiController_nativeGetParsedOAuth2Code(JNIEnv * env, jclass)
{
  std::string const & code = frm()->GetParsedOAuth2Code();
  return jni::ToJavaString(env, code);
}

JNIEXPORT jstring Java_app_organicmaps_sdk_api_ApiController_nativeGetParsedBackUrl(JNIEnv * env, jclass)
{
  std::string const & backUrl = frm()->GetParsedBackUrl();
  return (backUrl.empty()) ? nullptr : jni::ToJavaString(env, backUrl);
}

JNIEXPORT jdoubleArray Java_app_organicmaps_sdk_api_ApiController_nativeGetParsedCenterLatLon(JNIEnv * env, jclass)
{
  ms::LatLon const center = frm()->GetParsedCenterLatLon();
  if (!center.IsValid())
    return nullptr;

  double latlon[] = {center.m_lat, center.m_lon};
  jdoubleArray jLatLon = env->NewDoubleArray(2);
  env->SetDoubleArrayRegion(jLatLon, 0, 2, latlon);

  return jLatLon;
}
}

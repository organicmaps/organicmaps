#include "Track.hpp"

#include "app/organicmaps/sdk/Framework.hpp"
#include "app/organicmaps/sdk/bookmarks/data/ElevationInfo.hpp"
#include "app/organicmaps/sdk/bookmarks/data/Metadata.hpp"
#include "app/organicmaps/sdk/bookmarks/data/TrackStatistics.hpp"
#include "app/organicmaps/sdk/core/jni_helper.hpp"

jobject CreateTrack(JNIEnv * env, place_page::Info const & info, jni::TScopedLocalObjectArrayRef const & jrawTypes,
                    jni::TScopedLocalRef const & routingPointInfo)
{
  // clang-format off
  static jmethodID const ctorId = jni::GetConstructorID(env, g_trackClazz,
    "("
    "J"                                               // categoryId
    "J"                                               // trackId
    "Ljava/lang/String;"                              // title
    "Ljava/lang/String;"                              // secondaryTitle
    "Ljava/lang/String;"                              // subtitle
    "Ljava/lang/String;"                              // address
    "Lapp/organicmaps/sdk/routing/RoutePointInfo;"    // routePointInfo
    "I"                                               // openingMode
    "Ljava/lang/String;"                              // wikiArticle
    "Ljava/lang/String;"                              // osmDescription
    "[Ljava/lang/String;"                             // rawTypes
    "I"                                               // color
    "Lapp/organicmaps/sdk/util/Distance;"             // length
    "D"                                               // lat
    "D"                                               // lon
    ")V"
  );
  // clang-format on

  auto const trackId = info.GetTrackId();
  auto const track = frm()->GetBookmarkManager().GetTrack(trackId);
  ms::LatLon const ll = info.GetLatLon();
  // clang-format off
  jobject mapObject = env->NewObject(g_trackClazz, ctorId,
    static_cast<jlong>(track->GetGroupId()),
    static_cast<jlong>(trackId),
    jni::ToJavaStringWithSupplementalCharsFix(env, info.GetTitle()),
    jni::ToJavaStringWithSupplementalCharsFix(env, info.GetSecondaryTitle()),
    jni::ToJavaStringWithSupplementalCharsFix(env, info.GetSubtitle()),
    jni::ToJavaStringWithSupplementalCharsFix(env, info.GetSecondarySubtitle()),
    routingPointInfo.get(),
    info.GetOpeningMode(),
    jni::ToJavaString(env, info.GetWikiDescription()),
    jni::ToJavaString(env, info.GetOSMDescription()),
    jrawTypes.get(),
    track->GetColor(0).GetARGB(),
    ToJavaDistance(env, platform::Distance::CreateFormatted(track->GetLengthMeters())),
    static_cast<jdouble>(ll.m_lat),
    static_cast<jdouble>(ll.m_lon)
  );
  // clang-format on

  if (info.HasMetadata())
    InjectMetadata(env, g_mapObjectClazz, mapObject, info);
  return mapObject;
}

extern "C"
{
JNIEXPORT jstring Java_app_organicmaps_sdk_bookmarks_data_Track_nativeGetDescription(JNIEnv * env, jclass, jlong id)
{
  return jni::ToJavaString(env, frm()->GetBookmarkManager().GetTrack(static_cast<kml::TrackId>(id))->GetDescription());
}

JNIEXPORT jobject Java_app_organicmaps_sdk_bookmarks_data_Track_nativeGetElevationInfo(JNIEnv * env, jclass, jlong id)
{
  auto const & track = frm()->GetBookmarkManager().GetTrack(id);
  auto const & elevationInfo = track->GetElevationInfo();
  return track->GetElevationInfo().has_value() ? ToJavaElevationInfo(env, elevationInfo.value()) : nullptr;
}

JNIEXPORT jobject Java_app_organicmaps_sdk_bookmarks_data_Track_nativeGetStatistics(JNIEnv * env, jclass, jlong id)
{
  return ToJavaTrackStatistics(env, frm()->GetBookmarkManager().GetTrack(id)->GetStatistics());
}

JNIEXPORT jobject Java_app_organicmaps_sdk_bookmarks_data_Track_nativeGetElevationActivePointCoordinates(JNIEnv * env,
                                                                                                         jclass,
                                                                                                         jlong trackId)
{
  auto const & trackInfo = frm()->GetBookmarkManager().GetTrackSelectionInfo(trackId);
  auto const latlon = mercator::ToLatLon(trackInfo.m_trackPoint);
  return ToJavaElevationInfoPoint(env, latlon);
}

JNIEXPORT void Java_app_organicmaps_sdk_bookmarks_data_Track_nativeSetParams(JNIEnv * env, jclass, jlong id,
                                                                             jstring name, jint color, jstring descr)
{
  auto const * nTrack = frm()->GetBookmarkManager().GetTrack(static_cast<kml::TrackId>(id));
  CHECK(nTrack, ("Track must not be null with id:", id));

  kml::TrackData trackData(nTrack->GetData());
  auto const trkName = jni::ToNativeString(env, name);
  kml::SetDefaultStr(trackData.m_name, trkName);
  kml::SetDefaultStr(trackData.m_description, jni::ToNativeString(env, descr));

  uint8_t alpha = ExtractByte(color, 3);
  trackData.m_layers[0].m_color.m_rgba = static_cast<uint32_t>(shift(color, 8) + alpha);

  g_framework->ReplaceTrack(static_cast<kml::TrackId>(id), trackData);
}

JNIEXPORT void Java_app_organicmaps_sdk_bookmarks_data_Track_nativeChangeColor(JNIEnv *, jclass, jlong id, jint color)
{
  uint8_t const alpha = ExtractByte(color, 3);
  g_framework->ChangeTrackColor(static_cast<kml::TrackId>(id), static_cast<dp::Color>(shift(color, 8) + alpha));
}

JNIEXPORT void Java_app_organicmaps_sdk_bookmarks_data_Track_nativeChangeCategory(JNIEnv *, jclass, jlong oldCat,
                                                                                  jlong newCat, jlong trackId)
{
  g_framework->MoveTrack(static_cast<kml::TrackId>(trackId), static_cast<kml::MarkGroupId>(oldCat),
                         static_cast<kml::MarkGroupId>(newCat));
}

JNIEXPORT jdouble Java_app_organicmaps_sdk_bookmarks_data_Track_nativeGetElevationCurPositionDistance(JNIEnv *, jclass,
                                                                                                      jlong trackId)
{
  auto const & bm = frm()->GetBookmarkManager();
  return static_cast<jdouble>(bm.GetElevationMyPosition(static_cast<kml::TrackId>(trackId)));
}

JNIEXPORT jdouble Java_app_organicmaps_sdk_bookmarks_data_Track_nativeGetElevationActivePointDistance(JNIEnv *, jclass,
                                                                                                      jlong trackId)
{
  auto & bm = frm()->GetBookmarkManager();
  return static_cast<jdouble>(bm.GetElevationActivePoint(static_cast<kml::TrackId>(trackId)));
}
}  // extern "C"

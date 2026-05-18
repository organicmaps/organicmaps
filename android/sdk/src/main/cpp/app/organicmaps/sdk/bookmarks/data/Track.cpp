#include "Track.hpp"

#include "app/organicmaps/sdk/Framework.hpp"
#include "app/organicmaps/sdk/bookmarks/data/ElevationInfo.hpp"
#include "app/organicmaps/sdk/bookmarks/data/Metadata.hpp"
#include "app/organicmaps/sdk/bookmarks/data/TrackStatistics.hpp"
#include "app/organicmaps/sdk/core/jni_helper.hpp"

namespace
{
jobjectArray BuildTrackCandidatesArray(JNIEnv * env, place_page::Info const & info)
{
  auto const & candidates = info.GetTrackCandidates();
  if (candidates.empty())
    return nullptr;

  jni::TScopedLocalClassRef candidateClazz(
      env, env->FindClass("app/organicmaps/sdk/bookmarks/data/TrackSelectionCandidate"));
  static jmethodID const candidateCtor = jni::GetConstructorID(env, candidateClazz.get(), "(JLjava/lang/String;IZ)V");

  jobjectArray result = env->NewObjectArray(static_cast<jsize>(candidates.size()), candidateClazz.get(), nullptr);

  auto const isRelationTrack = info.IsRelationTrack();
  auto const selectedTrackId = info.GetTrackId();
  auto const & selectedRelationId = info.GetTrackRelationId();
  for (size_t i = 0; i < candidates.size(); ++i)
  {
    auto const & c = candidates[i];
    bool const isSelected = isRelationTrack ? c.m_relationId == selectedRelationId : c.m_trackId == selectedTrackId;
    jni::TScopedLocalRef title(env, jni::ToJavaStringWithSupplementalCharsFix(env, c.m_title));
    jni::TScopedLocalRef candidate(
        env, env->NewObject(candidateClazz.get(), candidateCtor, static_cast<jlong>(c.m_trackId), title.get(),
                            static_cast<jint>(c.m_color.GetARGB()), static_cast<jboolean>(isSelected)));
    env->SetObjectArrayElement(result, static_cast<jsize>(i), candidate.get());
  }
  return result;
}
}  // namespace

jobject CreateTrack(JNIEnv * env, place_page::Info const & info, jni::TScopedLocalObjectArrayRef const & jrawTypes,
                    jni::TScopedLocalRef const & routingPointInfo)
{
  // clang-format off
  static jmethodID const ctorId = jni::GetConstructorID(env, g_trackClazz,
    "("
    "J"                                               // categoryId
    "J"                                               // trackId
    "Z"                                               // isRelationTrack
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
    "[Lapp/organicmaps/sdk/bookmarks/data/TrackSelectionCandidate;"  // candidates
    ")V"
  );
  // clang-format on

  auto const trackId = info.GetTrackId();
  auto const track = frm()->GetBookmarkManager().GetTrack(trackId);
  ms::LatLon const ll = info.GetLatLon();
  jni::TScopedLocalObjectArrayRef candidatesArray(env, BuildTrackCandidatesArray(env, info));
  // clang-format off
  jobject mapObject = env->NewObject(g_trackClazz, ctorId,
    static_cast<jlong>(track->GetGroupId()),
    static_cast<jlong>(trackId),
    static_cast<jboolean>(info.IsRelationTrack()),
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
    static_cast<jdouble>(ll.m_lon),
    candidatesArray.get()
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
  auto const * track = frm()->GetBookmarkManager().GetTrack(id);
  auto const * info = track->GetElevationInfo();
  return (info ? ToJavaElevationInfo(env, *info) : nullptr);
}

JNIEXPORT jobject Java_app_organicmaps_sdk_bookmarks_data_Track_nativeGetStatistics(JNIEnv * env, jclass, jlong id)
{
  return ToJavaTrackStatistics(env, frm()->GetBookmarkManager().GetTrack(id)->GetStatistics());
}

JNIEXPORT jdoubleArray Java_app_organicmaps_sdk_bookmarks_data_Track_nativeGetElevationActivePointCoordinates(
    JNIEnv * env, jclass, jlong trackId)
{
  auto const & trackInfo = frm()->GetBookmarkManager().GetTrackSelectionInfo(trackId);
  auto const latlon = mercator::ToLatLon(trackInfo.m_trackPoint);
  jdoubleArray result = env->NewDoubleArray(2);
  jdouble coords[] = {latlon.m_lat, latlon.m_lon};
  env->SetDoubleArrayRegion(result, 0, 2, coords);
  return result;
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

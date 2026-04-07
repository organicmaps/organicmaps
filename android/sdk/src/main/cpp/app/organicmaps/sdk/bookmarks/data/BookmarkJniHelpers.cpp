#include "app/organicmaps/sdk/bookmarks/data/BookmarkJniHelpers.hpp"
#include "app/organicmaps/sdk/Framework.hpp"
#include "app/organicmaps/sdk/core/jni_helper.hpp"
#include "app/organicmaps/sdk/util/Distance.hpp"

#include "map/bookmark.hpp"
#include "map/track.hpp"

#include "kml/type_utils.hpp"
#include "kml/types.hpp"

#include "base/math.hpp"

#include "platform/distance.hpp"

namespace bookmark_jni
{
namespace
{
jclass g_bookmarkInfoClass = nullptr;
jmethodID g_bookmarkInfoConstructor = nullptr;
jclass g_trackClass = nullptr;
jmethodID g_trackConstructor = nullptr;
}  // namespace

void PrepareClassRefs(JNIEnv * env)
{
  if (g_bookmarkInfoClass != nullptr)
    return;

  g_bookmarkInfoClass = jni::GetGlobalClassRef(env, "app/organicmaps/sdk/bookmarks/data/BookmarkInfo");
  g_bookmarkInfoConstructor = jni::GetConstructorID(env, g_bookmarkInfoClass,
                                                    "("
                                                    "J"                   // categoryId
                                                    "J"                   // bookmarkId
                                                    "Ljava/lang/String;"  // title
                                                    "Ljava/lang/String;"  // description
                                                    "Ljava/lang/String;"  // featureType
                                                    "I"                   // color
                                                    "I"                   // iconType
                                                    "Lapp/organicmaps/sdk/bookmarks/data/ParcelablePointD;"  // coords
                                                    "D"                                                      // scale
                                                    "Ljava/lang/String;"                                     // address
                                                    ")V");

  g_trackClass = jni::GetGlobalClassRef(env, "app/organicmaps/sdk/bookmarks/data/Track");
  g_trackConstructor = jni::GetConstructorID(env, g_trackClass,
                                             "("
                                             "J"                                    // id
                                             "J"                                    // categoryId
                                             "Z"                                    // isRelationTrack
                                             "Ljava/lang/String;"                   // title
                                             "Lapp/organicmaps/sdk/util/Distance;"  // length
                                             "I"                                    // color
                                             ")V");

  jni::HandleJavaException(env);
}

jobject CreateBookmarkInfo(JNIEnv * env, Bookmark const & bookmark)
{
  auto const title = jni::ToJavaString(env, bookmark.GetPreferredName());
  auto const description = jni::ToJavaString(env, bookmark.GetDescription());
  auto const featureType = jni::ToJavaString(env, kml::GetLocalizedFeatureType(bookmark.GetData().m_featureTypes));
  auto const color = static_cast<jint>(kml::kColorIndexMap[base::E2I(bookmark.GetColor())]);
  auto const iconType = static_cast<jint>(bookmark.GetData().m_icon);
  auto const coords = jni::GetNewParcelablePointD(env, bookmark.GetPivot());
  auto const scale = static_cast<jdouble>(bookmark.GetScale());
  auto const address = jni::ToJavaString(env, frm()->GetAddressAtPoint(bookmark.GetPivot()).FormatAddress());

  return env->NewObject(g_bookmarkInfoClass, g_bookmarkInfoConstructor, static_cast<jlong>(bookmark.GetGroupId()),
                        static_cast<jlong>(bookmark.GetId()), title, description, featureType, color, iconType, coords,
                        scale, address);
}

jobject CreateTrack(JNIEnv * env, Track const & track)
{
  auto const isRelationTrack = static_cast<jboolean>(track.GetId() == kml::kTempRelationTrackId);
  return env->NewObject(
      g_trackClass, g_trackConstructor, static_cast<jlong>(track.GetId()), static_cast<jlong>(track.GetGroupId()),
      isRelationTrack, jni::ToJavaString(env, track.GetName()),
      ToJavaDistance(env, platform::Distance::CreateFormatted(track.GetLengthMeters())), track.GetColor(0).GetARGB());
}
}  // namespace bookmark_jni

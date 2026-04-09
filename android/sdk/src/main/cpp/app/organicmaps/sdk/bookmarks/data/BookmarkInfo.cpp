#include "BookmarkInfo.hpp"

#include "app/organicmaps/sdk/Framework.hpp"
#include "app/organicmaps/sdk/core/jni_helper.hpp"

#include "map/bookmark.hpp"

#include "kml/type_utils.hpp"
#include "kml/types.hpp"

#include "base/math.hpp"

jobject CreateBookmarkInfo(JNIEnv * env, Bookmark const & bookmark)
{
  static jmethodID const ctorId =
      jni::GetConstructorID(env, g_bookmarkInfoClazz,
                            "("
                            "J"                                                      // categoryId
                            "J"                                                      // bookmarkId
                            "Ljava/lang/String;"                                     // title
                            "Ljava/lang/String;"                                     // description
                            "Ljava/lang/String;"                                     // featureType
                            "I"                                                      // color
                            "I"                                                      // iconType
                            "Lapp/organicmaps/sdk/bookmarks/data/ParcelablePointD;"  // coords
                            "D"                                                      // scale
                            "Ljava/lang/String;"                                     // address
                            ")V");

  auto const title = jni::ToJavaString(env, bookmark.GetPreferredName());
  auto const description = jni::ToJavaString(env, bookmark.GetDescription());
  auto const featureType = jni::ToJavaString(env, kml::GetLocalizedFeatureType(bookmark.GetData().m_featureTypes));
  auto const color = static_cast<jint>(kml::kColorIndexMap[base::E2I(bookmark.GetColor())]);
  auto const iconType = static_cast<jint>(bookmark.GetData().m_icon);
  auto const coords = jni::GetNewParcelablePointD(env, bookmark.GetPivot());
  auto const scale = static_cast<jdouble>(bookmark.GetScale());
  auto const address = jni::ToJavaString(env, frm()->GetAddressAtPoint(bookmark.GetPivot()).FormatAddress());

  return env->NewObject(g_bookmarkInfoClazz, ctorId, static_cast<jlong>(bookmark.GetGroupId()),
                        static_cast<jlong>(bookmark.GetId()), title, description, featureType, color, iconType, coords,
                        scale, address);
}

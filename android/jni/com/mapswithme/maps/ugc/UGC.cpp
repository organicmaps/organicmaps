#include "../../core/jni_helper.hpp"
#include "../Framework.hpp"

#include "map/place_page_info.hpp"

#include "ugc/api.hpp"
#include "ugc/types.hpp"

#include "indexer/feature_decl.hpp"

#include "base/logging.hpp"

namespace
{
class FeatureIdBuilder
{
public:
  FeatureID Build(JNIEnv * env, jobject obj)
  {
    Init(env);

    jstring jcountryName = static_cast<jstring>(env->GetObjectField(obj, m_countryName));
    jlong jversion = env->GetLongField(obj, m_version);
    jint jindex = env->GetIntField(obj, m_index);

    auto const countryName = jni::ToNativeString(env, jcountryName);
    auto const version = static_cast<int64_t>(jversion);
    auto const index = static_cast<uint32_t>(jindex);

    auto const & ix = g_framework->GetIndex();
    auto const id = ix.GetMwmIdByCountryFile(platform::CountryFile(countryName));
    return FeatureID(id, index);
  }

private:
  void Init(JNIEnv * env)
  {
    if (m_initialized)
      return;

    m_class = jni::GetGlobalClassRef(env, "com/mapswithme/maps/bookmarks/data/FeatureId");
    m_countryName = env->GetFieldID(m_class, "mMwmName", "Ljava/lang/String;");
    m_version = env->GetFieldID(m_class, "mMwmVersion", "J");
    m_index = env->GetFieldID(m_class, "mFeatureIndex", "I");

    m_initialized = true;
  }

  bool m_initialized = false;

  jclass m_class;
  jfieldID m_countryName;
  jfieldID m_version;
  jfieldID m_index;
} g_builder;

class JavaBridge
{
public:
  void OnResult(JNIEnv * env, ugc::UGC const & ugc)
  {
    Init(env);
    jni::TScopedLocalRef result(env, ToJavaUGC(env, ugc));
    env->CallStaticVoidMethod(m_ugcClass, m_onResult, result.get());
  }

private:
  jobject ToJavaUGC(JNIEnv * env, ugc::UGC const & ugc)
  {
    jni::TScopedLocalObjectArrayRef ratings(env, ToJavaRatings(env, ugc.m_ratings));
    jni::TScopedLocalObjectArrayRef reviews(env, ToJavaReviews(env, ugc.m_reviews));

    jobject result = env->NewObject(m_ugcClass, m_ugcCtor, ratings.get(), ugc.m_aggRating,
                                    reviews.get());
    ASSERT(result, ());
    return result;
  }

  jobjectArray ToJavaRatings(JNIEnv * env, std::vector<ugc::RatingRecord> const & ratings)
  {
    size_t const n = ratings.size();
    jobjectArray result = env->NewObjectArray(n, m_ratingClass, nullptr);
    for (size_t i = 0; i < n; ++i)
    {
      jni::TScopedLocalRef rating(env, ToJavaRating(env, ratings[i]));
      env->SetObjectArrayElement(result, i, rating.get());
    }
    return result;
  }

  jobjectArray ToJavaReviews(JNIEnv * env, std::vector<ugc::Review> const & reviews)
  {
    size_t const n = reviews.size();
    jobjectArray result = env->NewObjectArray(n, m_reviewClass, nullptr);
    for (size_t i = 0; i < n; ++i)
    {
      jni::TScopedLocalRef review(env, ToJavaReview(env, reviews[i]));
      env->SetObjectArrayElement(result, i, review.get());
    }
    return result;
  }

  jobject ToJavaRating(JNIEnv * env, ugc::RatingRecord const & ratingRecord)
  {
    jni::TScopedLocalRef name(env, jni::ToJavaString(env, ratingRecord.m_key.m_key));
    jobject result = env->NewObject(m_ratingClass, m_ratingCtor, name.get(), ratingRecord.m_value);
    ASSERT(result, ());
    return result;
  }

  jobject ToJavaReview(JNIEnv * env, ugc::Review const & review)
  {
    jni::TScopedLocalRef text(env, jni::ToJavaString(env, review.m_text.m_text));
    jni::TScopedLocalRef author(env, jni::ToJavaString(env, review.m_author.m_name));
    jobject result = env->NewObject(m_reviewClass, m_reviewCtor, text.get(), author.get(),
                                    static_cast<jlong>(ugc::DaysAgo(review.m_time)));
    ASSERT(result, ());
    return result;
  }

  void Init(JNIEnv * env)
  {
    if (m_initialized)
      return;

    m_ugcClass = jni::GetGlobalClassRef(env, "com/mapswithme/maps/ugc/UGC");
    m_ugcCtor = jni::GetConstructorID(
        env, m_ugcClass,
        "([Lcom/mapswithme/maps/ugc/UGC$Rating;F[Lcom/mapswithme/maps/ugc/UGC$Review;)V");
    m_onResult = jni::GetStaticMethodID(env, m_ugcClass, "onUGCReceived",
                                        "(Lcom/mapswithme/maps/ugc/UGC;)V");

    m_ratingClass = jni::GetGlobalClassRef(env, "com/mapswithme/maps/ugc/UGC$Rating");
    m_ratingCtor = jni::GetConstructorID(env, m_ratingClass, "(Ljava/lang/String;F)V");

    m_reviewClass = jni::GetGlobalClassRef(env, "com/mapswithme/maps/ugc/UGC$Review");
    m_reviewCtor =
        jni::GetConstructorID(env, m_reviewClass, "(Ljava/lang/String;Ljava/lang/String;J)V");

    m_initialized = true;
  }

  bool m_initialized = false;

  jclass m_ugcClass;
  jmethodID m_ugcCtor;
  jmethodID m_onResult;

  jclass m_ratingClass;
  jmethodID m_ratingCtor;

  jclass m_reviewClass;
  jmethodID m_reviewCtor;
} g_bridge;
}  // namespace

extern "C" {
JNIEXPORT void JNICALL Java_com_mapswithme_maps_ugc_UGC_requestUGC(JNIEnv * env, jclass /* clazz */,
                                                                   jobject featureId)
{
  auto const fid = g_builder.Build(env, featureId);
  g_framework->RequestUGC(fid, [&](ugc::UGC const & ugc, ugc::UGCUpdate const & update) {
    JNIEnv * e = jni::GetEnv();
    g_bridge.OnResult(e, ugc);
  });
}
}

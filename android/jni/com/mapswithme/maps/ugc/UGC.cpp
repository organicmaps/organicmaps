#include "com/mapswithme/maps/Framework.hpp"
#include "com/mapswithme/core/jni_helper.hpp"

#include "map/place_page_info.hpp"

#include "ugc/api.hpp"
#include "ugc/types.hpp"

#include "indexer/feature_decl.hpp"

#include "base/logging.hpp"

#include <utility>

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
  void OnResult(JNIEnv * env, ugc::UGC const & ugc, ugc::UGCUpdate const & ugcUpdate)
  {
    Init(env);
    jni::TScopedLocalRef ugcResult(env, ToJavaUGC(env, ugc));
    jni::TScopedLocalRef ugcUpdateResult(env, ToJavaUGCUpdate(env, ugcUpdate));

    std::string formattedRating = place_page::rating::GetRatingFormatted(ugc.m_totalRating);
    jni::TScopedLocalRef jrating(env, jni::ToJavaString(env, formattedRating));

    env->CallStaticVoidMethod(m_ugcClass, m_onResult, ugcResult.get(), ugcUpdateResult.get(),
                              ToImpress(ugc.m_totalRating), jrating.get());
  }

  const int ToImpress(float const rating)
  {
    return static_cast<int>(place_page::rating::GetImpress(rating));
  }

  ugc::UGCUpdate ToNativeUGCUpdate(JNIEnv * env, jobject ugcUpdate)
  {
    Init(env);

    jobjectArray jratings = static_cast<jobjectArray>(env->GetObjectField(ugcUpdate, m_ratingArrayFieldId));
    int const length = env->GetArrayLength(jratings);
    std::vector<ugc::RatingRecord> records(length);
    for (int i = 0; i < length; i++)
    {
      jobject jrating = env->GetObjectArrayElement(jratings, i);

      jstring name = static_cast<jstring>(env->GetObjectField(jrating, m_ratingNameFieldId));
      ugc::TranslationKey key(jni::ToNativeString(env, name));

      jfloat value = env->GetFloatField(jrating, m_ratingValueFieldId);
      auto const ratingValue = static_cast<float>(value);

      records.emplace_back(std::move(key), std::move(ratingValue));
    }
    jstring jtext = static_cast<jstring>(env->GetObjectField(ugcUpdate, m_ratingTextFieldId));
    // TODO: use lang parameter correctly.
    ugc::Text text(jni::ToNativeString(env, jtext), 1);
    jlong jtime = env->GetLongField(ugcUpdate, m_updateTimeFieldId);
    uint64_t timeSec = static_cast<uint64_t>(jtime / 1000);
    return ugc::UGCUpdate(records, text, std::chrono::system_clock::from_time_t(timeSec));
  }

private:
  jobject ToJavaUGC(JNIEnv * env, ugc::UGC const & ugc)
  {
    jni::TScopedLocalObjectArrayRef ratings(env, ToJavaRatings(env, ugc.m_ratings));
    jni::TScopedLocalObjectArrayRef reviews(env, ToJavaReviews(env, ugc.m_reviews));

    jobject result = nullptr;
    if (!ugc.IsEmpty())
      result = env->NewObject(m_ugcClass, m_ugcCtor, ratings.get(), ugc.m_totalRating,
                              reviews.get(), ugc.m_basedOn);
    return result;
  }

  jobject ToJavaUGCUpdate(JNIEnv * env, ugc::UGCUpdate const & ugcUpdate)
  {
    jni::TScopedLocalObjectArrayRef ratings(env, ToJavaRatings(env, ugcUpdate.m_ratings));
    jni::TScopedLocalRef text(env, jni::ToJavaString(env, ugcUpdate.m_text.m_text));

    jobject result = nullptr;
    if (!ugcUpdate.IsEmpty())
      result = env->NewObject(m_ugcUpdateClass, m_ugcUpdateCtor, ratings.get(),
                              text.get(), ugc::ToMillisecondsSinceEpoch(ugcUpdate.m_time));
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
    jni::TScopedLocalRef author(env, jni::ToJavaString(env, review.m_author));
    jobject result = env->NewObject(m_reviewClass, m_reviewCtor, text.get(), author.get(),
                                    ugc::ToMillisecondsSinceEpoch(review.m_time), review.m_rating,
                                    ToImpress(review.m_rating));
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
        "([Lcom/mapswithme/maps/ugc/UGC$Rating;F[Lcom/mapswithme/maps/ugc/UGC$Review;I)V");
    m_onResult = jni::GetStaticMethodID(env, m_ugcClass, "onUGCReceived",
                                        "(Lcom/mapswithme/maps/ugc/UGC;Lcom/mapswithme/maps/ugc/UGCUpdate;ILjava/lang/String;)V");

    m_ratingClass = jni::GetGlobalClassRef(env, "com/mapswithme/maps/ugc/UGC$Rating");
    m_ratingCtor = jni::GetConstructorID(env, m_ratingClass, "(Ljava/lang/String;F)V");

    m_reviewClass = jni::GetGlobalClassRef(env, "com/mapswithme/maps/ugc/UGC$Review");
    m_reviewCtor =
        jni::GetConstructorID(env, m_reviewClass, "(Ljava/lang/String;Ljava/lang/String;JFI)V");

    m_ugcUpdateClass = jni::GetGlobalClassRef(env, "com/mapswithme/maps/ugc/UGCUpdate");
    m_ugcUpdateCtor = jni::GetConstructorID(
        env, m_ugcUpdateClass, "([Lcom/mapswithme/maps/ugc/UGC$Rating;Ljava/lang/String;J)V");
    m_ratingArrayFieldId = env->GetFieldID(m_ugcUpdateClass, "mRatings", "[Lcom/mapswithme/maps/ugc/UGC$Rating;");
    m_ratingTextFieldId = env->GetFieldID(m_ugcUpdateClass, "mText", "Ljava/lang/String;");
    m_updateTimeFieldId = env->GetFieldID(m_ugcUpdateClass, "mTimeMillis", "J");
    m_ratingNameFieldId = env->GetFieldID(m_ratingClass, "mName", "Ljava/lang/String;");
    m_ratingValueFieldId = env->GetFieldID(m_ratingClass, "mValue", "F");
    m_initialized = true;
  }

  bool m_initialized = false;

  jclass m_ugcClass;
  jmethodID m_ugcCtor;

  jclass m_ugcUpdateClass;
  jmethodID m_ugcUpdateCtor;
  jfieldID m_ratingArrayFieldId;
  jfieldID m_ratingTextFieldId;
  jfieldID m_updateTimeFieldId;
  jfieldID m_ratingNameFieldId;
  jfieldID m_ratingValueFieldId;

  jmethodID m_onResult;

  jclass m_ratingClass;
  jmethodID m_ratingCtor;

  jclass m_reviewClass;
  jmethodID m_reviewCtor;
} g_bridge;
}  // namespace

extern "C" {
JNIEXPORT
void JNICALL Java_com_mapswithme_maps_ugc_UGC_requestUGC(JNIEnv * env, jclass /* clazz */,
                                                                   jobject featureId)
{
  auto const fid = g_builder.Build(env, featureId);
  g_framework->RequestUGC(fid, [&](ugc::UGC const & ugc, ugc::UGCUpdate const & update) {
    JNIEnv * e = jni::GetEnv();
    g_bridge.OnResult(e, ugc, update);
  });
}

JNIEXPORT
void JNICALL Java_com_mapswithme_maps_ugc_UGC_setUGCUpdate(JNIEnv * env, jclass /* clazz */,
                                                           jobject featureId, jobject ugcUpdate)
{
  auto const fid = g_builder.Build(env, featureId);
  ugc::UGCUpdate update = g_bridge.ToNativeUGCUpdate(env, ugcUpdate);
  g_framework->SetUGCUpdate(fid, update);
}

JNIEXPORT
void JNICALL Java_com_mapswithme_maps_ugc_UGC_nativeUploadUGC(JNIEnv * env, jclass /* clazz */)
{
  g_framework->UploadUGC();
}
}

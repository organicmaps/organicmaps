#include "Metadata.hpp"

#include "app/organicmaps/sdk/core/jni_helper.hpp"

jobject CreateMetadata(JNIEnv * env, osm::MapObject const & mapObject)
{
  static jclass clazz = jni::GetGlobalClassRef(env, "app/organicmaps/sdk/bookmarks/data/Metadata");
  static jmethodID ctorId = jni::GetConstructorID(env, clazz, "()V");
  static jmethodID putMethodId = env->GetMethodID(clazz, "put", "(ILjava/lang/String;)V");
  ASSERT(putMethodId, ("Metadata.put method not found"));

  jobject metadataObject = env->NewObject(clazz, ctorId);

  mapObject.ForEachMetadataReadable([env, &metadataObject](osm::MapObject::MetadataID id, std::string const & meta)
  {
    /// @todo Make separate processing of non-string values like FMD_DESCRIPTION.
    /// Actually, better to call separate getters instead of ToString processing.
    if (!meta.empty())
    {
      jni::TScopedLocalRef metaString(env, jni::ToJavaString(env, meta));
      env->CallVoidMethod(metadataObject, putMethodId, static_cast<jint>(id), metaString.get());
    }
  });

  return metadataObject;
}

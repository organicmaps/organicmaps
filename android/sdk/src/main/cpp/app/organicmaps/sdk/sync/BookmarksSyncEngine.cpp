#include "app/organicmaps/sdk/Framework.hpp"
#include "app/organicmaps/sdk/core/ScopedLocalRef.hpp"
#include "app/organicmaps/sdk/core/jni_helper.hpp"

#include "map/bookmark_helpers.hpp"
#include "map/bookmarks_sync/sync_engine.hpp"

#include "base/logging.hpp"

namespace
{
using namespace bookmarks_sync;

char constexpr kCloudFileSig[] = "Lapp/organicmaps/sdk/sync/CloudFile;";

struct JavaIds
{
  explicit JavaIds(JNIEnv * env)
  {
    jni::ScopedLocalRef<jclass> const store(env, env->FindClass("app/organicmaps/sdk/sync/CloudStore"));
    m_getHashAlgorithm =
        env->GetMethodID(store.get(), "getHashAlgorithm", "()Lapp/organicmaps/sdk/sync/HashAlgorithm;");
    m_list = env->GetMethodID(store.get(), "list", "()[Lapp/organicmaps/sdk/sync/CloudFile;");
    m_stat = env->GetMethodID(store.get(), "stat", "(Ljava/lang/String;)Lapp/organicmaps/sdk/sync/CloudFile;");
    m_createGeneration = env->GetMethodID(store.get(), "createGeneration", "()Ljava/lang/String;");
    m_download = env->GetMethodID(store.get(), "download", "(Ljava/lang/String;Ljava/lang/String;)Z");
    m_upload = env->GetMethodID(store.get(), "upload",
                                "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)Lapp/organicmaps/sdk/sync/"
                                "CloudFile;");
    m_moveToTrash = env->GetMethodID(store.get(), "moveToTrash", "(Ljava/lang/String;)V");
    m_purgeTrash = env->GetMethodID(store.get(), "purgeTrash", "(J)V");
    m_deleteAll = env->GetMethodID(store.get(), "deleteAll", "(Ljava/lang/String;)V");

    jni::ScopedLocalRef<jclass> const enumClass(env, env->FindClass("java/lang/Enum"));
    m_ordinal = env->GetMethodID(enumClass.get(), "ordinal", "()I");

    jni::ScopedLocalRef<jclass> const file(env, env->FindClass("app/organicmaps/sdk/sync/CloudFile"));
    m_id = env->GetFieldID(file.get(), "id", "Ljava/lang/String;");
    m_name = env->GetFieldID(file.get(), "name", "Ljava/lang/String;");
    m_version = env->GetFieldID(file.get(), "version", "Ljava/lang/String;");
    m_hash = env->GetFieldID(file.get(), "hash", "Ljava/lang/String;");
    m_createdTimeMs = env->GetFieldID(file.get(), "createdTimeMs", "J");

    m_cloudException = jni::GetGlobalClassRef(env, "app/organicmaps/sdk/sync/CloudException");
    m_errorCode = env->GetMethodID(m_cloudException, "errorCode", "()I");
    m_errorMessage = env->GetMethodID(m_cloudException, "errorMessage", "()Ljava/lang/String;");
    m_exceptionFromNative = env->GetStaticMethodID(m_cloudException, "fromNative",
                                                   "(ILjava/lang/String;)Lapp/organicmaps/sdk/sync/CloudException;");

    m_engine = jni::GetGlobalClassRef(env, "app/organicmaps/sdk/sync/BookmarksSyncEngine");
    m_passResult =
        env->GetStaticMethodID(m_engine, "passResult", "(IILjava/lang/String;)Lapp/organicmaps/sdk/sync/PassResult;");

    jni::ScopedLocalRef<jclass> const gate(env, env->FindClass("app/organicmaps/sdk/sync/LocalChangesGate"));
    m_canApply = env->GetMethodID(gate.get(), "canApply", "()Z");
    CHECK(!env->ExceptionCheck(), (jni::DescribeException()));
  }

  jmethodID m_getHashAlgorithm, m_list, m_stat, m_createGeneration, m_download, m_upload, m_moveToTrash, m_purgeTrash,
      m_deleteAll, m_ordinal, m_errorCode, m_errorMessage, m_exceptionFromNative, m_passResult, m_canApply;
  jfieldID m_id, m_name, m_version, m_hash, m_createdTimeMs;
  jclass m_cloudException, m_engine;
};

// Classes are looked up on a thread that entered the core from Java, which sees the app class loader.
JavaIds const & GetIds(JNIEnv * env)
{
  static JavaIds const ids(env);
  return ids;
}

/// The Kotlin store, called on the thread of the pass.
class JniCloudStore : public CloudStore
{
public:
  JniCloudStore(JNIEnv * env, jobject store) : m_env(env), m_store(store), m_ids(GetIds(env)) {}

  std::optional<HashAlgorithm> GetHashAlgorithm() const override
  {
    jni::ScopedLocalRef<jobject> const algorithm(m_env, m_env->CallObjectMethod(m_store, m_ids.m_getHashAlgorithm));
    Rethrow();
    if (!algorithm.get())
      return {};
    return static_cast<HashAlgorithm>(m_env->CallIntMethod(algorithm.get(), m_ids.m_ordinal));
  }

  std::vector<CloudFile> List() override
  {
    jni::ScopedLocalRef<jobjectArray> const array(
        m_env, static_cast<jobjectArray>(m_env->CallObjectMethod(m_store, m_ids.m_list)));
    Rethrow();
    std::vector<CloudFile> files;
    jsize const size = m_env->GetArrayLength(array.get());
    files.reserve(size);
    for (jsize i = 0; i < size; ++i)
    {
      jni::ScopedLocalRef<jobject> const file(m_env, m_env->GetObjectArrayElement(array.get(), i));
      files.push_back(ToCloudFile(file.get()));
    }
    return files;
  }

  std::optional<CloudFile> Stat(std::string const & id) override
  {
    jni::ScopedLocalRef<jstring> const jid(m_env, jni::ToJavaString(m_env, id));
    jni::ScopedLocalRef<jobject> const file(m_env, m_env->CallObjectMethod(m_store, m_ids.m_stat, jid.get()));
    Rethrow();
    if (!file.get())
      return {};
    return ToCloudFile(file.get());
  }

  std::string CreateGeneration() override
  {
    jni::ScopedLocalRef<jstring> const id(
        m_env, static_cast<jstring>(m_env->CallObjectMethod(m_store, m_ids.m_createGeneration)));
    Rethrow();
    return jni::ToNativeString(m_env, id.get());
  }

  bool Download(std::string const & id, std::string const & destPath) override
  {
    jni::ScopedLocalRef<jstring> const jid(m_env, jni::ToJavaString(m_env, id));
    jni::ScopedLocalRef<jstring> const jpath(m_env, jni::ToJavaString(m_env, destPath));
    bool const downloaded = m_env->CallBooleanMethod(m_store, m_ids.m_download, jid.get(), jpath.get());
    Rethrow();
    return downloaded;
  }

  CloudFile Upload(std::string const & name, std::string const & srcPath,
                   std::optional<std::string> const & existingId) override
  {
    jni::ScopedLocalRef<jstring> const jname(m_env, jni::ToJavaString(m_env, name));
    jni::ScopedLocalRef<jstring> const jpath(m_env, jni::ToJavaString(m_env, srcPath));
    jni::ScopedLocalRef<jstring> const jid(m_env, existingId ? jni::ToJavaString(m_env, *existingId) : nullptr);
    jni::ScopedLocalRef<jobject> const file(
        m_env, m_env->CallObjectMethod(m_store, m_ids.m_upload, jname.get(), jpath.get(), jid.get()));
    Rethrow();
    return ToCloudFile(file.get());
  }

  void MoveToTrash(std::string const & id) override
  {
    jni::ScopedLocalRef<jstring> const jid(m_env, jni::ToJavaString(m_env, id));
    m_env->CallVoidMethod(m_store, m_ids.m_moveToTrash, jid.get());
    Rethrow();
  }

  void PurgeTrash(int64_t trashedBeforeMs) override
  {
    m_env->CallVoidMethod(m_store, m_ids.m_purgeTrash, static_cast<jlong>(trashedBeforeMs));
    Rethrow();
  }

  void DeleteAll(std::optional<std::string> const & generationId) override
  {
    jni::ScopedLocalRef<jstring> const jid(m_env, generationId ? jni::ToJavaString(m_env, *generationId) : nullptr);
    m_env->CallVoidMethod(m_store, m_ids.m_deleteAll, jid.get());
    Rethrow();
  }

private:
  void Rethrow() const
  {
    jni::ScopedLocalRef<jthrowable> const e(m_env, m_env->ExceptionOccurred());
    if (!e.get())
      return;
    m_env->ExceptionClear();
    if (m_env->IsInstanceOf(e.get(), m_ids.m_cloudException))
    {
      auto const error = static_cast<SyncError>(m_env->CallIntMethod(e.get(), m_ids.m_errorCode));
      jni::ScopedLocalRef<jstring> const message(
          m_env, static_cast<jstring>(m_env->CallObjectMethod(e.get(), m_ids.m_errorMessage)));
      throw CloudException(error, "CloudException", jni::ToNativeString(m_env, message.get()));
    }
    // A bug in the store, like an unexpected response it doesn't check.
    auto const description = jni::ToNativeString(m_env, e.get());
    LOG(LERROR, ("Unexpected exception in the cloud store", description));
    throw CloudException(SyncError::Other, "CloudException", description);
  }

  std::string GetString(jobject object, jfieldID field) const
  {
    jni::ScopedLocalRef<jstring> const value(m_env, static_cast<jstring>(m_env->GetObjectField(object, field)));
    return value.get() ? jni::ToNativeString(m_env, value.get()) : std::string{};
  }

  CloudFile ToCloudFile(jobject file) const
  {
    jni::ScopedLocalRef<jstring> const hash(m_env, static_cast<jstring>(m_env->GetObjectField(file, m_ids.m_hash)));
    return {.m_id = GetString(file, m_ids.m_id),
            .m_name = GetString(file, m_ids.m_name),
            .m_version = GetString(file, m_ids.m_version),
            .m_hash = hash.get() ? std::optional(jni::ToNativeString(m_env, hash.get())) : std::nullopt,
            .m_createdTimeMs = m_env->GetLongField(file, m_ids.m_createdTimeMs)};
  }

  JNIEnv * m_env;
  jobject m_store;
  JavaIds const & m_ids;
};

SyncEngine & GetEngine(jlong handle)
{
  return *reinterpret_cast<SyncEngine *>(handle);
}

void ThrowJavaException(JNIEnv * env, SyncError error, std::string const & message)
{
  auto const & ids = GetIds(env);
  jni::ScopedLocalRef<jstring> const jmessage(env, jni::ToJavaString(env, message));
  jni::ScopedLocalRef<jthrowable> const e(
      env, static_cast<jthrowable>(env->CallStaticObjectMethod(ids.m_cloudException, ids.m_exceptionFromNative,
                                                               static_cast<jint>(error), jmessage.get())));
  env->Throw(e.get());
}
}  // namespace

extern "C"
{
JNIEXPORT jlong Java_app_organicmaps_sdk_sync_BookmarksSyncEngine_nativeCreate(JNIEnv * env, jclass, jstring statePath,
                                                                               jstring stagingDir, jstring deviceName,
                                                                               jint hashAlgorithm)
{
  std::optional<HashAlgorithm> algorithm;
  if (hashAlgorithm >= 0)
    algorithm = static_cast<HashAlgorithm>(hashAlgorithm);
  // Lives as long as the process, like the provider it synchronizes with.
  auto * engine =
      new SyncEngine(frm()->GetBookmarkManager(), GetBookmarksDirectory(), jni::ToNativeString(env, statePath),
                     jni::ToNativeString(env, stagingDir), jni::ToNativeString(env, deviceName), algorithm);
  return reinterpret_cast<jlong>(engine);
}

JNIEXPORT jobject Java_app_organicmaps_sdk_sync_BookmarksSyncEngine_nativeRunPass(JNIEnv * env, jclass, jlong handle,
                                                                                  jobject store, jobject gate)
{
  auto const & ids = GetIds(env);
  JniCloudStore cloud(env, store);
  // Copies outlive the pass in reloads that core applies later, on the GUI thread.
  CanApply const canApply = [gate = jni::make_global_ref_safe(gate), method = ids.m_canApply]()
  {
    JNIEnv * env = jni::GetEnv();
    bool const canApply = env->CallBooleanMethod(*gate, method);
    return !jni::HandleJavaException(env) && canApply;
  };
  auto const result = GetEngine(handle).RunPass(cloud, canApply);
  jni::ScopedLocalRef<jstring> const message(env, jni::ToJavaString(env, result.m_message));
  return env->CallStaticObjectMethod(ids.m_engine, ids.m_passResult, static_cast<jint>(result.m_status),
                                     static_cast<jint>(result.m_error), message.get());
}

JNIEXPORT void Java_app_organicmaps_sdk_sync_BookmarksSyncEngine_nativeCancel(JNIEnv *, jclass, jlong handle)
{
  GetEngine(handle).Cancel();
}

JNIEXPORT jboolean Java_app_organicmaps_sdk_sync_BookmarksSyncEngine_nativeIsRunning(JNIEnv *, jclass, jlong handle)
{
  return GetEngine(handle).IsRunning();
}

JNIEXPORT jlong Java_app_organicmaps_sdk_sync_BookmarksSyncEngine_nativeGetLastSync(JNIEnv *, jclass, jlong handle)
{
  return GetEngine(handle).GetLastSyncMs().value_or(0);
}

JNIEXPORT void Java_app_organicmaps_sdk_sync_BookmarksSyncEngine_nativeForgetMissingLocalFiles(JNIEnv *, jclass,
                                                                                               jlong handle)
{
  try
  {
    GetEngine(handle).ForgetMissingLocalFiles();
  }
  catch (RootException const & e)
  {
    // The next pass compares the files as they are.
    LOG(LWARNING, ("Can't forget missing local files", e.Msg()));
  }
}

JNIEXPORT void Java_app_organicmaps_sdk_sync_BookmarksSyncEngine_nativeDeleteCloudData(JNIEnv * env, jclass,
                                                                                       jlong handle, jobject store)
{
  JniCloudStore cloud(env, store);
  try
  {
    GetEngine(handle).DeleteCloudData(cloud);
  }
  catch (CloudException const & e)
  {
    ThrowJavaException(env, e.GetError(), e.Msg());
  }
  catch (RootException const & e)
  {
    ThrowJavaException(env, SyncError::Other, e.Msg());
  }
}
}  // extern "C"

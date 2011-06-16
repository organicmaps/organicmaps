#include <string.h>
#include <jni.h>
#include <android/log.h>
#include <assert.h>


#define LOG(...)  __android_log_print(ANDROID_LOG_INFO, "main_native", __VA_ARGS__)

namespace jni_help
{
	jmethodID GetJavaMethodID(JNIEnv * env, jobject obj,
							char const * fn, char const * sig)
	{
		jclass cls = env->GetObjectClass(obj);
		jmethodID mid = env->GetMethodID(cls, fn, sig);
		assert(mid != 0);
		return mid;
	}

	class String
	{
		JNIEnv * m_env;
		jstring m_s;
		jchar const * m_ret;
	public:
		String(JNIEnv * env, jstring s) : m_env(env), m_s(s)
		{
			LOG("String constructor");
			m_ret = m_env->GetStringChars(m_s, NULL);
		}
		~String()
		{
			LOG("String destructor");
			m_env->ReleaseStringChars(m_s, m_ret);
		}
	};
}


extern "C"
{
	JNIEXPORT jstring JNICALL
	Java_com_mapswithme_maps_MWMActivity_stringsJNI(JNIEnv * env, jobject thiz, jstring s)
	{
		LOG("Enter stringsJNI");

		{
			jni_help::String str(env, s);
		}

		LOG("Leave stringsJNI");
		return env->NewStringUTF("Return string from JNI.");
	}

	JNIEXPORT void JNICALL
	Java_com_mapswithme_maps_MWMActivity_callbackFromJNI(JNIEnv * env, jobject thiz)
	{
		LOG("Enter callbackFromJNI");

		env->CallVoidMethod(thiz, jni_help::GetJavaMethodID(env, thiz,
				"callbackVoid", "()V"));
		env->CallVoidMethod(thiz, jni_help::GetJavaMethodID(env, thiz,
				"callbackString", "(Ljava/lang/String;)V"),
				env->NewStringUTF("Pass string from JNI."));

		LOG("Leave callbackFromJNI");
	}
}

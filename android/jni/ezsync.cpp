/**
 * $Id$
 * @author caoyangmin(caoyangmin@gmail.com)
 * @brief
 */
#include <jni.h>
#include <android/log.h>
#include "com_caoym_ezsync_Client.h"
#include "com_caoym_ezsync_AppSetting.h"
#include "jni_object.h"
#include "ezsync/update_method2.h"
#include "ezsync/delete_method.h"
#include "ezsync/commit_method2.h"
#include "ezsync/file_storage.h"
#include "ezsync/client2.h"
#include "ezsync/file_sync_client_builder2.h"
#include "ezsync_client_wrap.h"
#include "ezsync/sqlite_sync_client_builder2.h"
#include "jstring_map.h"
#include "jmerge_strategy.h"
#include "ezsync/verify_transfer_result.h"
#include "../src/transfer/TransferBase.h"

//
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved)
{
	/*
	JNIEnv* env = NULL;
	jint result = -1;

	if ((*vm)->GetEnv(vm, (void**) &env, JNI_VERSION_1_4) != JNI_OK) {
		return -1;
	}

	result = JNI_VERSION_1_4;
	*/
	ezsync::TransferBase::set_jvm(vm);
	return JNI_VERSION_1_4;
}


#define CATCH_AND_THROW_JAVA_EXCEPTION() \
    catch(const ezsync::RequestUpdateException&e){\
        JNIException::Throw(env, e.what(),"com/caoym/ezsync/RequestUpdateException");\
    }\
    catch(const ezsync::AuthFailException&e){\
        JNIException::Throw(env, e.what(),"com/caoym/ezsync/AuthFailException");\
    }\
    catch (const std::exception&e) {\
        JNIException::Throw(env, e.what());\
    }

/*
 * Class:     com_caoym_ezsync_Client
 * Method:    commint
 * Signature: (Ljava/lang/String;)V
 */JNIEXPORT jobjectArray JNICALL Java_com_caoym_ezsync_Client_commit(JNIEnv *env,
		jobject thiz, jstring path) {
	try {
		ezsync::ChangedEntries committed =
				GetJNIObj<IEzsyncClientWrap>(env, thiz)->commit(
						JUTF8String(env, path));
		jobjectArray res = env->NewObjectArray(committed.size(),
				env->FindClass("com/caoym/ezsync/Changed"), 0);
		int i = 0;
		for (ezsync::ChangedEntries::iterator it = committed.begin();
				it != committed.end(); it++) {
			JObject op(env, "com/caoym/ezsync/Changed");
			op.set(&JNIEnv::SetIntField, "operator", "I", it->second.op);
			op.set(&JNIEnv::SetObjectField, "key", "Ljava/lang/String;",
					JUTF8String(env, it->first).j_str());
			env->SetObjectArrayElement(res, i++, op.get());
		}
		return res;
	}
	CATCH_AND_THROW_JAVA_EXCEPTION();
	return 0;
}

/*
 * Class:     com_caoym_ezsync_Client
 * Method:    delete
 * Signature: (Ljava/lang/String;)V
 */JNIEXPORT void JNICALL Java_com_caoym_ezsync_Client_delete(JNIEnv *env,
		jobject thiz, jstring path) {
	try {
		GetJNIObj<IEzsyncClientWrap>(env, thiz)->del(
				JUTF8String(env, path).c_str());
	}
	CATCH_AND_THROW_JAVA_EXCEPTION();
}
class JUpdateListener: public IUpdateListener {
public:
	JUpdateListener(JNIEnv *env, jobject listener) :
			_listener(env, "com/caoym/ezsync/UpdateListener", listener, false),
			_env(env) {

	}
	virtual bool post_update(const ezsync::ChangedEntries&changed) {

		jobjectArray res = _env->NewObjectArray(changed.size(),
				_env->FindClass("com/caoym/ezsync/Changed"), 0);
		int i = 0;
		for (ezsync::ChangedEntries::const_iterator it = changed.begin();
				it != changed.end(); it++) {
			JObject op(_env, "com/caoym/ezsync/Changed");
			op.set(&JNIEnv::SetIntField, "operator", "I", (jint) it->second.op);
			op.set(&JNIEnv::SetObjectField, "key", "Ljava/lang/String;",
					JUTF8String(_env, it->first).j_str());
			_env->SetObjectArrayElement(res, i++, op.get());
		}

		return (ezsync::UseWitch) _listener.call<jboolean>(
				&JNIEnv::CallBooleanMethod, "postUpdate",
				"([Lcom/caoym/ezsync/Changed;)Z", res);
	}
private:
	JNIEnv * _env;
	JObject _listener;

};

/*
 * Class:     com_caoym_ezsync_Client
 * Method:    update
 * Signature: (Lcom/caoym/ezsync/MergeStrategy;Ljava/lang/String;Ljava/lang/Boolean;)V
 */JNIEXPORT jobjectArray JNICALL Java_com_caoym_ezsync_Client_update(JNIEnv *env,
		jobject thiz, jobject strategy, jstring include, jstring except, jobject listener) {

	try {
		JMergeStrategy jstrategy(env, strategy);
		ezsync::ChangedEntries updated;
		if (listener) {
			JUpdateListener jlistener(env, listener);
			updated = GetJNIObj<IEzsyncClientWrap>(env, thiz)->update(jstrategy,
					JUTF8String(env, include), JUTF8String(env, except),
					&jlistener);
		} else {
			updated = GetJNIObj<IEzsyncClientWrap>(env, thiz)->update(jstrategy,
					JUTF8String(env, include), JUTF8String(env, except),NULL);
		}

		jobjectArray res = env->NewObjectArray(updated.size(),
				env->FindClass("com/caoym/ezsync/Changed"), 0);
		int i = 0;
		for (ezsync::ChangedEntries::iterator it = updated.begin();
				it != updated.end(); it++) {
			JObject op(env, "com/caoym/ezsync/Changed");
			op.set(&JNIEnv::SetIntField, "operator", "I", (jint) it->second.op);
			op.set(&JNIEnv::SetObjectField, "key", "Ljava/lang/String;",
					JUTF8String(env, it->first).j_str());
			env->SetObjectArrayElement(res, i++, op.get());
		}

		return res;
	}
	CATCH_AND_THROW_JAVA_EXCEPTION();
	return 0;
}

/*
 * Class:     com_caoym_ezsync_Client
 * Method:    destroyJniObj
 * Signature: ()V
 */JNIEXPORT void JNICALL Java_com_caoym_ezsync_Client_destroyJniObj(JNIEnv *env,
		jobject thiz) {
	DetachJNIObj<IEzsyncClientWrap>(env, thiz);
}

/*
 * Class:     com_caoym_ezsync_Client
 * Method:    createJniObj
 * Signature: (Ljava/util/Map;)V
 */JNIEXPORT void JNICALL Java_com_caoym_ezsync_Client_createJniObj(JNIEnv *env,
		jobject thiz, jobject config) {
	IEzsyncClientWrap * client_wrap = 0;
	try {
		std::map<std::string, std::string> map = JStringMap(env, config);
		if (map["local_type"] == "file") {
			ezsync::FileSyncClient* client = 0;
			std::map<std::string, std::string> map = JStringMap(env, config);
			client = ezsync::FileSyncClientBuilder2::build(map);
			client_wrap = new EzsyncClientWrap<ezsync::FileSyncClient2>(client);

		} else if (map["local_type"] == "sqlite") {
			ezsync::SqliteSyncClient* client = 0;
			std::map<std::string, std::string> map = JStringMap(env, config);
			client = ezsync::SqliteSyncClientBuilder2::build(map);
			client_wrap = new EzsyncClientWrap<ezsync::SqliteSyncClient2>(client);

		} else {
			JNIException::Throw(env, "unknown ezsync type");
			return;
		}
		ezsync::log::Debug debug;
		debug << "build client: \r\n";
		for (std::map<std::string, std::string>::iterator it = map.begin();
				it != map.end(); it++) {
			debug << it->first << ": " << it->second << "\r\n";
		}
		debug << "OK";
		AttachJNIObj(env, thiz, client_wrap);
	}
	CATCH_AND_THROW_JAVA_EXCEPTION();
}
/*
 * Class:     com_caoym_ezsync_Client
 * Method:    cancel
 * Signature: ()V
 */JNIEXPORT void JNICALL Java_com_caoym_ezsync_Client_cancel(JNIEnv *env,
		jobject thiz) {
	try {
		GetJNIObj<IEzsyncClientWrap>(env, thiz)->cancel();
	}
	CATCH_AND_THROW_JAVA_EXCEPTION();
}
 /*
  * Class:     com_caoym_ezsync_Client
  * Method:    upgradeLocalStorage
  * Signature: ()V
  */JNIEXPORT void JNICALL Java_com_caoym_ezsync_Client_upgradeLocalStorage(JNIEnv *env,
 		jobject thiz) {
 	try {
 		GetJNIObj<IEzsyncClientWrap>(env, thiz)->upgradeLocalStorage();
 	}
 	CATCH_AND_THROW_JAVA_EXCEPTION();
 }
unsigned int g_log_flag = 0;
bool g_enable_log = false;
std::string g_log_path;
std::string g_log_name;

namespace ezsync {
namespace log {
void print_log(const std::string& tag, const std::string& str) {
	/* ANDROID_LOG_UNKNOWN = 0,
	 ANDROID_LOG_DEFAULT,
	 ANDROID_LOG_VERBOSE,
	 ANDROID_LOG_DEBUG,
	 ANDROID_LOG_INFO,
	 ANDROID_LOG_WARN,
	 ANDROID_LOG_ERROR,
	 ANDROID_LOG_FATAL,
	 ANDROID_LOG_SILENT,   */
	static const char* tags[] = { "", "", "", "debug", "info", "warn", "error",
			"", "" };
	int itag = std::find(tags, tags + (sizeof(tags) / sizeof(tags[0])), tag)
			- tags;

	//return ;
	if (g_enable_log && (itag >= g_log_flag+2)) {
		__android_log_print(itag, "ezsync", "%s", str.c_str());
		time_t now;
		time(&now);
		char temp[32] = { 0 };
		strftime(temp, 100, ".log.%Y-%m-%d", gmtime(&now));
		std::ofstream f(g_log_path + "/" + g_log_name + temp,
				std::ios::binary | std::ios::app);

		if (f.is_open()) {
			time_t rawtime;
			time(&rawtime);
			f << ctime(&rawtime) << "\t" << tag << "\t" << str << "\r\n";
			f.close();
		} else {
			__android_log_print(ANDROID_LOG_DEBUG, "ezsync",
					"open log file %s,failed with %d",
					(g_log_path + "/" + g_log_name + temp).c_str(), errno);
		}
	}

}
}
}

/*
 * Class:     com_caoym_ezsync_AppSetting
 * Method:    enableLog
 * Signature: (Ljava/lang/String;Ljava/lang/String;I)V
 */JNIEXPORT void JNICALL Java_com_caoym_ezsync_AppSetting_enableLog(JNIEnv *env,
		jobject, jstring path, jstring name, jint flag) {
	if (g_enable_log)
		return;

	g_log_flag = flag;
	g_log_path = JUTF8String(env, path).c_str();
	g_log_name = JUTF8String(env, name).c_str();
	g_enable_log = true;
}

/*
 * Class:     com_caoym_ezsync_AppSetting
 * Method:    disableLog
 * Signature: ()V
 */JNIEXPORT void JNICALL Java_com_caoym_ezsync_AppSetting_disableLog(JNIEnv *,
		jobject) {
	g_enable_log = false;
}

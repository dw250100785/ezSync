
/**
* $Id: jstring_map.h 631 2014-03-12 11:07:48Z caoyangmin $
* @author caoyangmin(caoyangmin@gmail.com)
* @brief JObject
*/

#pragma once

#include <jni.h>
#include <string>
#include <map>
#include "ezsync/verify_true.h"
// TODO: 捕获JNI异常
/**
 * java 对象的简单C++包装
 */
class JObject {
public:

	JObject(JNIEnv*env, const char* class_name):
				_env(env), _class_name(class_name),_auto_release(true){
			_class = env->FindClass(class_name);
			VERIFY_TRUE(!!_class) << "FindClass " << class_name << " failed";
			jmethodID init = env->GetMethodID(_class, "<init>", "()V");
			_object = env->NewObject(_class, init);
			if(_object == 0) {_env->DeleteLocalRef(_class);_class=0;}
			VERIFY_TRUE(!!_object) << "NewObject " << class_name << " failed";
		}

	JObject(JNIEnv*env, const char* class_name, jobject object,bool auto_release = true):
			_env(env), _class_name(class_name), _object(object),_auto_release(auto_release) {
		_class = env->FindClass(class_name);
		VERIFY_TRUE(!!_class) << "FindClass " << class_name << " failed";
	}

	~JObject(){
		if(_auto_release&&_object) _env->DeleteLocalRef(_object);
		if(_class) _env->DeleteLocalRef(_class);
	}
	jobject get(){
		return _object;
	}
	jobject detach(){
		jobject temp = 0;
		std::swap(_object,temp);
		return temp;
	}
	template<typename C, typename T>
	void set(C c, const char* field, const char* def, const T& t) {
		jfieldID id = _env->GetFieldID(_class, field, def);
		VERIFY_TRUE(!!id) << "GetFieldID " << field << " " << def << " failed";
		(_env->*c)(_object, id, t);
	}
	template<typename R,typename C>
	R call(C c, const char* method, const char* def) {
		return (_env->*c)(_object, get_method_id(method, def));
	}

	template<typename R,typename C, typename A1>
	R call(C c, const char* method, const char* def, A1 a1) {
		return (_env->*c)(_object, get_method_id(method, def), a1);
	}

	template<typename R,typename C, typename A1, typename A2>
	R call(C c, const char* method, const char* def, A1 a1,A2 a2) {
		return (_env->*c)(_object, get_method_id(method, def), a1, a2);
	}

	template<typename R,typename C, typename A1, typename A2, typename A3>
		R call(C c, const char* method, const char* def, A1 a1,A2 a2,A3 a3) {
			return (_env->*c)(_object, get_method_id(method, def), a1, a2, a3);
		}
private:
	jmethodID get_method_id(const char* method, const char* def) {
		jmethodID m = _env->GetMethodID(_class, method, def);
		VERIFY_TRUE(!!m) << "GetMethodID " << method << " " << def << " failed";
		return m;
	}
	jclass _class;
	JNIEnv*_env;
	const char* _class_name;
	jobject _object;
	bool _auto_release;
};

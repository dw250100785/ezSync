
/**
* $Id: jstring_map.h 631 2014-03-12 11:07:48Z caoyangmin $
* @author caoyangmin(caoyangmin@gmail.com)
* @brief JObject JStringMap
*/

#pragma once

#include <jni.h>
#include <string>
#include "ezsync/verify_true.h"

/**
 * java字符串对象的C++包装
 */
class JUTF8String {
public:
	JUTF8String(JNIEnv*env, const std::string& str):
		_str(str),
		_j(0),
		_auto_release(true),
		_env(env){
		_j = env->NewStringUTF(str.c_str());
		VERIFY_TRUE(!!_j) << "NewStringUTF failed";
	}
	JUTF8String(JNIEnv*env,jstring j):
			_j(j),
			_auto_release(false),
			_env(env){
			if(_j){
				const char*p = env->GetStringUTFChars(_j,0);
				_str = p?p:"";
				env->ReleaseStringUTFChars(_j,p);
			}
	}
	~JUTF8String() {
		if(_auto_release&&_j) _env->DeleteLocalRef(_j);
	}
	operator jstring ()const{
		return _j;
	}
	operator const char* ()const{
		return _str.c_str();
	}
	operator const std::string& ()const{
			return _str;
		}
	const char* c_str()const{
			return _str.c_str();
	}
	jstring j_str()const{
		return _j;
	}
private:
	JNIEnv*_env;
	bool _auto_release;
	jstring _j;
	std::string _str;
};

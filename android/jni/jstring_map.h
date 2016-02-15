
/**
* $Id: jstring_map.h 631 2014-03-12 11:07:48Z caoyangmin $
* @author caoyangmin(caoyangmin@gmail.com)
* @brief JStringMap
*/

#pragma once

#include <jni.h>
#include <string>
#include <map>
#include "ezsync/verify_true.h"
#include "jobject.h"
#include "jutf8string.h"

// TODO: 捕获JNI异常
/**
 * java map对象的简单C++包装
 */
class JStringMap {
public:
	JStringMap(JNIEnv*env, jobject map) {
		JObject map_obj(env,"java/util/Map",map,false);
		jobject set = map_obj.call<jobject>(&JNIEnv::CallObjectMethod,"entrySet","()Ljava/util/Set;");
		VERIFY_TRUE (!!set )<< "CallObjectMethod entrySet failed";
		JObject set_obj(env,"java/util/Set",set);
		jobject iter = set_obj.call<jobject>(&JNIEnv::CallObjectMethod,"iterator","()Ljava/util/Iterator;");
		VERIFY_TRUE (!!iter ) << "CallObjectMethod iterator failed";
		JObject iterator_obj(env,"java/util/Iterator",iter);
		while (iterator_obj.call<jboolean>(&JNIEnv::CallBooleanMethod,"hasNext", "()Z")) {
			jobject entry = iterator_obj.call<jobject>(&JNIEnv::CallObjectMethod, "next","()Ljava/lang/Object;");
			JObject entry_obj(env,"java/util/Map$Entry",entry);
			std::string key = JUTF8String(env,(jstring)entry_obj.call<jobject>(&JNIEnv::CallObjectMethod,"getKey","()Ljava/lang/Object;"));
			std::string value = JUTF8String(env,(jstring)entry_obj.call<jobject>(&JNIEnv::CallObjectMethod,"getValue","()Ljava/lang/Object;"));
			_map[key] = value;
		}

	}
	operator const std::map<std::string,std::string>&()const{
		return _map;
	}
private:
	std::map<std::string,std::string> _map;
};

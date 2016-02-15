
/**
* $Id: commit_method2.h 1011 2014-08-05 06:45:25Z caoyangmin $ 
* @author caoyangmin(caoyangmin@gmail.com)
* @brief 用于绑定java实例和jni方法
*/
#pragma once
#include "jni_exception.h"
template<class T>
static T* AttachJNIObj(JNIEnv *env,jobject jthiz)
{
	jclass cls = env->GetObjectClass(jthiz);
	if (0 == cls)
	{
		JNIException::Throw(env,"GetObjectClass failed");
		return 0;
	}
	jfieldID l_id = env->GetFieldID(cls, "__jni_object", "J");
	if(l_id == 0) {
		JNIException::Throw(env,"GetFieldID __jni_object failed");
		return 0;
	}
	T*p = new T(); 
	env->SetLongField(jthiz,l_id,jlong(p) );
	return p;
}
template<class T>
static T* AttachJNIObj(JNIEnv *env,jobject jthiz,T*p)
{
	jclass cls = env->GetObjectClass(jthiz);
	if (0 == cls)
	{
		JNIException::Throw(env,"GetObjectClass failed");
		return 0;
	}
	jfieldID l_id = env->GetFieldID(cls, "__jni_object", "J");
	if(l_id == 0) {
		JNIException::Throw(env,"GetFieldID __jni_object failed");
		return 0;
	}
	env->SetLongField(jthiz,l_id,jlong(p) );
	return p;
}
template<class T>
static T* GetJNIObj(JNIEnv *env,jobject jthiz,bool no_throw = false)
{
	jclass cls = env->GetObjectClass(jthiz);

	if (0 == cls)
	{
		JNIException::Throw(env,"GetObjectClass failed");
	}
	jfieldID l_id = env->GetFieldID(cls, "__jni_object", "J");
	if(l_id == 0) JNIException::Throw(env,"GetFieldID __jni_object failed");
	jlong pointer = env->GetLongField(jthiz,l_id);
	if(pointer == 0 && !no_throw)
		JNIException::Throw(env,"__jni_object == 0");
	return (T*)pointer;
}

template<class T>
static bool DetachJNIObj(JNIEnv *env,jobject jthiz)
{
	T*pThis = GetJNIObj<T>(env,jthiz,true);
	if(pThis)
	{
		jclass cls = env->GetObjectClass(jthiz);
		if (0 == cls)
		{
			return false;
		}
		jfieldID l_id = env->GetFieldID(cls, "__jni_object", "J");
		if(l_id == 0) 
		{
			return false;
		}
		env->SetLongField(jthiz,l_id,0);

		delete pThis;
		return true;
	}
	return false;
}

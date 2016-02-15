/*! \file	$Id: jni_exception.h 684 2014-03-20 12:51:12Z caoyangmin $
 *  \author caoym
 *  \brief	JniException
 */
#pragma once
#include "jni.h"
#include <string>
//! throw java exception
class JNIException
{
public:
	JNIException(void);
	~JNIException(void);
	static void Throw(JNIEnv *env,const std::string& msg,const std::string& exception="java/lang/RuntimeException");
};

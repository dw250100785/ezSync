
#include "jni_exception.h"
#include "ezsync/log.h"
JNIException::JNIException(void)
{
}
JNIException::~JNIException(void)
{
}
void JNIException::Throw(JNIEnv *env,const std::string& e, const std::string& exception)
{
	ezsync::log::Warn()<<exception <<"\t" << e;
	jclass cls = (env)->FindClass(exception.c_str());
    if(!cls){
    	ezsync::log::Error()<<exception <<" class not found" << e;
        cls = (env)->FindClass("java/lang/RuntimeException");
    }
	(env)->ThrowNew( cls, e.c_str());

}

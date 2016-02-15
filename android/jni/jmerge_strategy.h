#include <jni.h>
#include <android/log.h>
#include "jni_object.h"
#include "ezsync/update_method2.h"
#include "jstring_map.h"

class JMergeStrategy:public IEzsyncMergeStrategy{
 public:
	 JMergeStrategy(JNIEnv *env,jobject strategy)
	 :_strategy(env,"com/caoym/ezsync/MergeStrategy",strategy,false)
	 ,_env(env){

	 }
     virtual ezsync::UseWitch merge(const std::string&key,const ezsync::Entry& local,const ezsync::Entry& remote){
    	 JObject local_obj(_env,"com/caoym/ezsync/Entry");
    	 JObject remote_obj(_env,"com/caoym/ezsync/Entry");
    	 local_obj.set(&JNIEnv::SetLongField,"modifiedTime","J",local.modified_time);
    	 local_obj.set(&JNIEnv::SetLongField,"size","J",local.size);
    	 local_obj.set(&JNIEnv::SetObjectField,"content","Ljava/lang/String;",JUTF8String(_env,local.content).j_str());
    	 remote_obj.set(&JNIEnv::SetLongField,"modifiedTime","J",remote.modified_time);
    	 remote_obj.set(&JNIEnv::SetLongField,"size","J",remote.size);
    	 remote_obj.set(&JNIEnv::SetObjectField,"content","Ljava/lang/String;",JUTF8String(_env,remote.content).j_str());
    	 return (ezsync::UseWitch)_strategy.call<jint>(
    			 &JNIEnv::CallIntMethod,
    			 "merge",
    			 "(Ljava/lang/String;Lcom/caoym/ezsync/Entry;Lcom/caoym/ezsync/Entry;)I",
    			 JUTF8String(_env,key).j_str(),
    			 local_obj.get(),
    			 remote_obj.get());
     }
 private:
     JNIEnv * _env;
     JObject _strategy;

 };

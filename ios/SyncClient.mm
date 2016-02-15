//
//  SyncClient.m
//  SampleCollection
//
//  Created by niustock on 16/1/29.
//  Copyright © 2016年 caoym. All rights reserved.
//

#import "SyncClient.h"


#include <iostream>

#include "ezsync/update_method2.h"
#include "ezsync/delete_method2.h"
#include "ezsync/commit_method2.h"
#include "ezsync/file_storage.h"
#include "ezsync/cloud_storage.h"

#include "ezsync/file_sync_client_builder2.h"
#include "ezsync/path_clip_box.h"
#include "ezsync_client_wrap.h"

using namespace ezsync;
class UseLocal:public IEzsyncMergeStrategy{
public:
    ezsync::UseWitch merge(const std::string& key,const ezsync::Entry&,const ezsync::Entry&){
        std::cout << key << " merged:use_local"<<std::endl;
        return USE_LOCAL;
    }
}_use_local;

namespace ezsync{
    namespace log
    {
        void print_log(const std::string& tag,const std::string& str){
            printf("--%s--\t%s\r\n",tag.c_str(),str.c_str());
        }
    }
}


@implementation SyncClient
{
    std::shared_ptr<IEzsyncClientWrap> _client;
}
- (instancetype)initWithLocal:(NSString*)local forType:(NSString*)type withHost:(NSString*)host withRemote:(NSString*)remote
{
    self = [super init];
    if (self) {
        std::map<std::string, std::string> map ;
        map["local_type"]=type.UTF8String;
        map["host"]=host.UTF8String;
        map["path"]="/";
        map["local_storage"]=local.UTF8String;
        map["remote_storage"]=remote.UTF8String;
        
        map["local_history"]=path.UTF8String;
        map["remote_history"]=remote.UTF8String;
        if(map["local_type"] == "file"){
            _client.reset(new EzsyncClientWrap<ezsync::FileSyncClient2>(ezsync::FileSyncClientBuilder2::build(map)));
        }else if(map["local_type"] == "sqlite"){
            _client.reset(new EzsyncClientWrap<ezsync::SqliteSyncClient2>(ezsync::SqliteSyncClientBuilder2::build(map)));
        }
    }
    return self;
}

-(void) update{
    _client->update(_use_local, "", "", NULL);
}
-(void) commit{
    _client->commit("");
}
-(void) delFile:(NSString*)name
{
    _client->del(name.UTF8String);
}


@end

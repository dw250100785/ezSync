/**
* $Id: file_sync_client_builder2.h 757 2014-04-08 06:49:22Z caoyangmin $ 
* @author caoyangmin(caoyangmin@gmail.com)
* @brief ezsync::FileSyncClientBuilder
*/
#pragma once

#include  <string>
#include  <map>
#include  <list>
#include <memory>
#include "ezsync/client2.h"
#include "ezsync/file_storage.h"
#include "ezsync/cloud_storage.h"
#include "ezsync/cloud_version_history2.h"
namespace ezsync{
    class TransferInterface;
    struct TransfersControl{
        std::list< std::shared_ptr<TransferInterface> > transfers;
        void reset();
        void cancel();
    };
    typedef Client2<FileStorage,CloudStorage,CloudVersionHistory2,TransfersControl> FileSyncClient2;

    class FileSyncClientBuilder2{
    public:
        static FileSyncClient2* build(const Config&conf);
    };
    typedef FileSyncClient2 FileSyncClient;
    typedef FileSyncClientBuilder2 FileSyncClientBuilder;
}

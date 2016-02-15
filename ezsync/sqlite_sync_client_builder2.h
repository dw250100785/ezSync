/**
* $Id: sqlite_sync_client_builder2.h 757 2014-04-08 06:49:22Z caoyangmin $ 
* @author caoyangmin(caoyangmin@gmail.com)
* @brief ezsync::SqliteSyncClientBuilder
*/
#pragma once
#include <map>
#include "ezsync/client2.h"
#include "ezsync/sqlite_storage.h"
#include "ezsync/cloud_version_history2.h"
#include "ezsync/cloud_storage.h"

namespace ezsync{
    class TransferInterface;


    typedef Client2<SqliteStorage,CloudStorage,CloudVersionHistory2,TransferInterface> SqliteSyncClient2;

    class SqliteSyncClientBuilder2{
    public:
        typedef std::map<std::string,std::string> Config;
        static SqliteSyncClient2* build(const Config&conf);
    };
    typedef SqliteSyncClient2 SqliteSyncClient;
}

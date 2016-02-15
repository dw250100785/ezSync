/**
* $Id: sqlite_sync_client.h 550 2014-02-27 15:00:40Z caoyangmin $ 
* @author caoyangmin(caoyangmin@gmail.com)
* @brief ezsync::Client
*/
#pragma once
#include <memory>
#include "transfer.h"
namespace ezsync{
    /**
     * ezsync 客户端
     * 
     * Client<...> client;
     * Method method;
     * client.execute(method);
     */
    class SqliteSyncClient:private Client<>
}

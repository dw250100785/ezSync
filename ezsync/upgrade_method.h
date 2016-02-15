/**
* $Id: update_method.h 717 2014-03-26 03:39:28Z caoyangmin $ 
* @author caoyangmin(caoyangmin@gmail.com)
* @brief
*/
#pragma once

#include <functional>
#include <list>
#include "verify_true.h"
#include "utility.h"
#include "ezsync/log.h"
#include "ezsync/transfer.h"
namespace ezsync{

    /**
     * 更新本地客户端数据结构
     * 目前只是更新sqlite表结构
     */
    class UpgradeMethod {
    public:

        UpgradeMethod() {
        }
        template<class Client>
        void execute(Client& client){
            log::Info() <<"attempt to upgrade...";
            client.get_local_storage().upgrade();
            log::Info() << "upgrade done";
        }
    };

}

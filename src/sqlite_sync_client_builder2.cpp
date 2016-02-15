/**
* $Id: sqlite_sync_client_builder2.cpp 970 2014-06-25 09:51:39Z caoyangmin $ 
* @author caoyangmin(caoyangmin@gmail.com)
* @brief ezsync::SqliteSyncClientBuilder
*/
#include "boost/algorithm/string.hpp"
#include "ezsync/sqlite_sync_client_builder2.h"
#include "ezsync/log.h"
#include "ezsync/verify_true.h"
#include "ezsync/verify_transfer_result.h"
#include "transfer/TransferRest.h"

namespace ezsync{

    //TODO: 验证ssl证书
	/**
	 * 构建sqlite同步客户端
	 */
    SqliteSyncClient2* SqliteSyncClientBuilder2::build(const std::map<std::string,std::string>&conf){
        std::map<std::string,std::string> temp = conf;
        std::shared_ptr<SqliteStorage> ls(new SqliteStorage(temp["local_storage"],temp["include"],temp["except"]));
        std::shared_ptr<TransferInterface> transfer;

        std::list<std::string> strs;
        headers_t headers;
        boost::algorithm::split( strs, temp["http_headers"], boost::is_any_of("\n") );
        std::for_each(strs.begin(),strs.end(),[&headers](std::string& it){
            std::vector<std::string> strs;
            boost::algorithm::split( strs, it, boost::is_any_of(":") );
            if(strs.size()==2){
                headers.push_back(std::make_pair(strs[0],strs[1]));
            }
        });

        TransferRest* rest = new TransferRest(temp["host"],temp["path"],headers,"",temp["host"]+"/batch",temp["encrypt"] != "0");
            
        std::shared_ptr<CloudStorage> re(new CloudStorage(transfer,temp["remote_storage"]));
        std::shared_ptr<CloudVersionHistory2> rvs(new CloudVersionHistory2(transfer,
            temp["local_history"]+".ezsync",temp["remote_storage"]));
        return new SqliteSyncClient2(ls,re,rvs,transfer);

    }
}

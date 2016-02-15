/**
* $Id: file_sync_client_builder2.cpp 970 2014-06-25 09:51:39Z caoyangmin $ 
* @author caoyangmin(caoyangmin@gmail.com)
* @brief ezsync::FileSyncClientBuilder
*/
#include  <list>
#include "boost/algorithm/string.hpp"
#include "ezsync/file_sync_client_builder2.h"
#include "ezsync/log.h"
#include "ezsync/verify_true.h"
#include "ezsync/verify_transfer_result.h"
#include "transfer/TransferPcs.h"
#include "transfer/TransferRest.h"
#include "transfer_with_logon.h"


namespace ezsync{

    void TransfersControl::reset(){
        std::for_each(transfers.begin(),transfers.end(),[](std::shared_ptr<TransferInterface>& it){if(it)it->reset();});
    }
    void TransfersControl::cancel(){
        std::for_each(transfers.begin(),transfers.end(),[](std::shared_ptr<TransferInterface>& it){if(it)it->cancel();});
    }
    //TODO: 验证ssl证书
	/**
	 * 文件同步客户端
	 */
    typedef std::list<std::pair<std::string, std::string> > headers_t;
    FileSyncClient2* FileSyncClientBuilder2::build(const std::map<std::string,std::string>&conf){

        std::map<std::string,std::string> temp = conf;
        std::shared_ptr<FileStorage> ls(new FileStorage(temp["local_storage"],temp["include"],temp["except"]));
        std::shared_ptr<TransferInterface> transfer;
        //boost::filesystem::create_directories(temp["local_history"]+"/.ezsync/cookie/");
        //std::string cookie_path = temp["local_history"]+"/.ezsync/cookie/"+get_text_md5(temp["host"]);

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
        std::shared_ptr<CloudStorage> re;
        std::shared_ptr<CloudVersionHistory2> rvs;
        std::shared_ptr<TransfersControl> control(new TransfersControl());

        std::string cookie = "BDUSS=" + temp["bduss"]+"; " +
            "UID=" + temp["uid"]+"; " +
            "DEVICEID=" + temp["device_id"]+"; ";

        //TODO: 保存所有cookie

        if(temp["remote_type"] == "REST"){//文件保存REST服务
            TransferRest* rest = new TransferRest(temp["host"],temp["path"],headers,"",temp["host"]+"/batch",temp["encrypt"] != "0");
            if(temp["note_token"].empty()){
                TransferWithLogon::OnLogonOK om_logon_ok = [cookie,rest](const std::string& access_token,const std::string& note_token)mutable{
                    rest->set_cookie(cookie+
                        "NOTETOKEN=" + note_token+"; ");
                };
                transfer.reset(new TransferWithLogon(std::shared_ptr<TransferInterface>(rest),
                    temp["host"],
                    cookie,
                    temp["local_history"]+"/.ezsync/bduss/",
                    headers,
                    "",
                    om_logon_ok,
                    temp["encrypt"] != "0")); // 使用SSL
            }else{
                rest->set_cookie(cookie+
                        "NOTETOKEN=" + temp["note_token"]+"; ");
                transfer.reset(rest);
            }


            re.reset(new CloudStorage(transfer,temp["remote_storage"]));
            rvs.reset(new CloudVersionHistory2(transfer,temp["local_history"],temp["remote_storage"]));
            control->transfers.push_back(transfer);

        }else if(temp["remote_type"] == "pcs"){//文件保存到pcs

            //版本信息不存pcs，因为pcs处理小文件太慢，且不能批量上传下载
            //用于数据保存
            TransferPcs* pcs = new TransferPcs(temp["host"],temp["path"],"",headers,"",true);
            if(temp["access_token"].empty()){
                TransferWithLogon::OnLogonOK om_logon_ok = [temp,pcs](const std::string& access_token,const std::string& note_token)mutable{
                    pcs->set_access_token(access_token);
                };
                transfer.reset(new TransferWithLogon(std::shared_ptr<TransferInterface>(pcs),
                    temp["logon_host"],
                    cookie,
                    temp["local_history"]+"/.ezsync/bduss/",
                    headers,
                    "",
                    om_logon_ok,
                    temp["encrypt"] != "0"));
            }else{
                pcs->set_access_token(temp["access_token"]);
                transfer.reset(pcs);
            }
            //用于版本信息保存
            TransferRest* rest(new TransferRest(temp["logon_host"],"/sql/v",headers,"",temp["logon_host"]+"/batch",temp["encrypt"] != "0"));
            std::shared_ptr<TransferInterface> rest_transfer;
            if(temp["note_token"].empty()){
                rest_transfer.reset(new TransferWithLogon(std::shared_ptr<TransferInterface>(rest),
                temp["logon_host"],
                cookie,
                temp["local_history"]+"/.ezsync/bduss/",
                headers,
                "",
                [cookie,rest](const std::string& access_token,const std::string& note_token)mutable{
                    rest->set_cookie(cookie+
                        "NOTETOKEN=" + note_token+"; ");
                },
                temp["encrypt"] != "0"));
            }else{
                rest->set_cookie(cookie+
                        "NOTETOKEN=" + temp["note_token"]+"; ");
                rest_transfer.reset(rest);
            }

            re.reset(new CloudStorage(transfer,temp["remote_storage"]));
            rvs.reset(new CloudVersionHistory2(rest_transfer,temp["local_history"],temp["remote_storage"]));
            control->transfers.push_back(rest_transfer);
            control->transfers.push_back(transfer);
        }else{
            VERIFY_TRUE(false)<< "unknown remote_type " <<temp["remote_type"];
        }

        return new FileSyncClient2(ls,re,rvs,control);

    }
}

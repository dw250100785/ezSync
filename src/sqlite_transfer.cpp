/**
* $Id: sqlite_transfer.cpp 970 2014-06-25 09:51:39Z caoyangmin $ 
* @author caoyangmin(caoyangmin@gmail.com)
* @brief sqlite数据库文件同步
*/
#include "boost/algorithm/string.hpp"
#include "transfer/TransferPcs.h"
#include "ezsync/log.h"
#include "ezsync/verify_true.h"
#include "ezsync/verify_transfer_result.h"
#include "transfer/TransferRest.h"
#include "ezsync/transfer.h"
#include "ezsync/cloud_storage.h"
#include "ezsync/sqlite_storage.h"
namespace ezsync{
    namespace transfer
    {
        template<>
        void copy(CloudStorage& from, const std::string& from_path,const Entry&src,SqliteStorage& to, const std::string& to_path){
            //路径需要编码,对于sqlite,其路径表示查询条件,如table?a=1&b=2,需要编码才能以文件路径保存
            std::string encoded_path;
            size_t pos = from_path.rfind('/');
            if(pos == std::string::npos){
                encoded_path = from_path;
            }else{
                encoded_path = from_path.substr(0,pos+1) + encode_key(from_path.substr(pos+1));
            }
            log::Debug()<<"copy cloud:"<< from.root() << "/" << encoded_path <<"("<<from_path<<")" << " -> " << to.root() << "/" << to_path;
            error_info_t err;
            std::string buffer;
            VERIFY_TRANSFER_RESULT(from.get_transfer().download_to_buffer(from.root()+"/"+encoded_path,buffer,err) ||
                FILE_NOT_EXIST == err.error_code ,err);
            if(FILE_NOT_EXIST == err.error_code){
                log::Warn()<<"remote entry " << from.root()+"/"+encoded_path <<" not exist";
            }else{
                to.set_entry(to_path,buffer);
            }
        }
        template<>
        void copy(SqliteStorage& from, const std::string& from_path,const Entry&src,CloudStorage& to, const std::string& to_path){
            Entry  e = from.get_entry(from_path);
            std::string encoded_path;
            size_t pos = to_path.rfind('/');
            if(pos == std::string::npos){
                encoded_path = to_path;
            }else{
                encoded_path = to_path.substr(0,pos+1) + encode_key(to_path.substr(pos+1));
            }
            log::Debug()<<"copy "<< from.root() << "/" << from_path <<" -> " << to.root() << "/" << encoded_path << "("<<to_path<<")" ;
            error_info_t err;
            VERIFY_TRANSFER_RESULT(to.get_transfer().upload_from_buffer(e.content,to.root()+"/"+encoded_path,false,err) || err.error_code == FILE_EXIST,err);
        }

        template<>
        void muti_copy(CloudStorage& from,SqliteStorage& to, const CopyItems& items){
            std::vector<std::pair<std::string,std::string> > to_copy;
            std::vector<return_info_t> result;

            //路径需要编码,对于sqlite,其路径表示查询条件,如table?a=1&b=2,需要编码才能以文件路径保存
            for(CopyItems::const_iterator it = items.begin(); it != items.end(); it++){
                std::string encoded_path;
                std::string from_path = std::get<0>(*it);
                size_t pos = from_path.rfind('/');
                if(pos == std::string::npos){
                    encoded_path = from_path;
                }else{
                    encoded_path = from_path.substr(0,pos+1) + encode_key(from_path.substr(pos+1));
                }
                to_copy.push_back(std::make_pair(from.root()+"/"+encoded_path,""));
            }
            error_info_t err;
            from.get_transfer().batch_download_to_buffer(to_copy,result,err);
            //VERIFY_TRUE(to_copy.size() == result.size() )<< "request count != respond count";
            to.begin_transaction();//启用事务
            bool all_success = true;
            for(size_t i=0; i< result.size(); i++){
                VERIFY_TRANSFER_RESULT(result[i].error_info.error_code != AUTH_FAILED,result[i].error_info);
                if(!result[i].is_success) {
                    if(FILE_NOT_EXIST != result[i].error_info.error_code )all_success = false;
                    log::Warn()<<"muti_copy cloud:"<< std::get<0>(items[i])  << " -> " << to.root() << "/" << std::get<2>(items[i]) << "failed "
                        << result[i].error_info.error_code <<" "<< result[i].error_info.error_msg
                        << " " << err.error_code<<" "<< err.error_msg;
                }else{
                    log::Debug()<<"muti_copy cloud:"<< std::get<0>(items[i])  << " -> " << to.root() << "/" << std::get<2>(items[i]) ;
                    to.set_entry(std::get<2>(items[i]),to_copy[i].second);
                }
            }
            to.commit_transaction();
            VERIFY_TRUE(all_success) << "something wrong";
            //VERIFY_TRUE(from.get_transfer().download_to_buffer(from.root()+"/"+encoded_path,buffer,err))<< err.error_code << " "<<err.error_msg;
            //VERIFY_TRUE(from.get_transfer().download_to_buffer(from.root()+"/"+encoded_path,buffer,err))<< err.error_code << " "<<err.error_msg;
            //to.set_entry(to_path,buffer);
        }
        template<>
        void muti_copy(SqliteStorage& from,CloudStorage& to,const CopyItems& items){//部分成功,记得继续处理之后的请求

            std::vector<std::pair<std::string,std::string> > to_copy;
            std::vector<return_info_t> result;

            //路径需要编码,对于sqlite,其路径表示查询条件,如table?a=1&b=2,需要编码才能以文件路径保存
            for(CopyItems::const_iterator it = items.begin(); it != items.end(); it++){
                Entry  e = from.get_entry(std::get<0>(*it));

                std::string encoded_path;
                std::string to_path = std::get<2>(*it);
                size_t pos = to_path.rfind('/');
                if(pos == std::string::npos){
                    encoded_path = to_path;
                }else{
                    encoded_path = to_path.substr(0,pos+1) + encode_key(to_path.substr(pos+1));
                }
                to_copy.push_back(std::make_pair(e.content,to.root()+"/"+encoded_path));
            }
            error_info_t err;
            VERIFY_TRANSFER_RESULT(to.get_transfer().batch_upload_from_buffer(to_copy,true,result,err),err);
            VERIFY_TRUE(to_copy.size() == result.size() )<< "request count != respond count";
            bool all_success = true;
            for(size_t i=0; i< result.size(); i++){
                VERIFY_TRANSFER_RESULT(result[i].is_success,result[i].error_info) ;
                log::Debug()<<"muti_copy "<< std::get<0>(items[i])  << " -> cloud:" << to.root() << "/" << std::get<2>(items[i]) ;
            }
        }

    }
}

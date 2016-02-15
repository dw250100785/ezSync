/**
* $Id: file_transfer.cpp 970 2014-06-25 09:51:39Z caoyangmin $ 
* @author caoyangmin(caoyangmin@gmail.com)
* @brief 文件同步实现
*/
#include  <list>
#include "boost/algorithm/string.hpp"
#include "ezsync/log.h"
#include "ezsync/verify_true.h"
#include "ezsync/verify_transfer_result.h"
#include "transfer/TransferPcs.h"
#include "transfer/TransferRest.h"
#include "ezsync/transfer.h"
#include "ezsync/file_storage.h"
#include "ezsync/cloud_storage.h"
namespace ezsync{
    namespace transfer{
        template<>
        void copy(FileStorage& from, const std::string& from_path,const Entry&src,CloudStorage& to, const std::string& to_path){
            log::Debug() << "copy " << from.root() << "/" <<from_path << " -> " <<" cloud:" <<to.root() << "/" << to_path;
            to.set_entry( to_path,from.root() + "/" +from_path);
        }
        template<>
        void copy(CloudStorage& from, const std::string& from_path,const Entry&src, FileStorage& to, const std::string& to_path){
            if(!to.include(to_path)){
                log::Warn() << "skip copy EXCEPT file" << "cloud:" <<from.root() << "/"<<from_path << " -> " << to.root() << "/" << to_path ;
                return ;
            }else{
                log::Debug() << "copy " << "cloud:" <<from.root() << "/"<<from_path << " -> " << to.root() << "/" << to_path ;
            }
            boost::filesystem::create_directories(to.root() +"/.ezsync/temp/");
            std::string temp_file = to.root() +"/.ezsync/temp/"+ get_text_md5(to_path);
            error_info_t e;
            VERIFY_TRANSFER_RESULT(from.get_transfer().download_to_file(from.root()+"/"+from_path,temp_file,e) || FILE_NOT_EXIST == e.error_code,e);
            if(e.error_code == FILE_NOT_EXIST){
                log::Warn()<<"remote file "<<from.root() << "/" << from_path << " not exist";
            }else{
                //TODO: boost /1_55可能不需要先remove
                boost::filesystem::path path(to.root() + "/" + to_path);
                boost::filesystem::remove(path);
                boost::filesystem::create_directories(path.parent_path());
                //TODO: 修改文件时间(部分Android系统文件无法修改文件时间)
                //boost::filesystem::last_write_time(temp_file,src.modified_time);
                boost::filesystem::rename(temp_file,path);
            }
        }
        template<>
        void muti_copy(FileStorage& from, CloudStorage& to, const CopyItems & items){
            /*for(CopyItems::const_iterator it = items.begin(); it != items.end(); it++){
                copy(from,std::get<0>(*it),std::get<1>(*it),to,std::get<2>(*it));
            }
            return ;*/
            //
            std::vector<std::pair<std::string,std::string> > to_copy;
            size_t size = 0;
            for(size_t i = 0 ; i < items.size(); i++){
                size += std::get<1>(items[i]).size;
                log::Debug() << std::get<0>(items[i]) <<" size: " << std::get<1>(items[i]).size << "/" << size << "B...";
                to_copy.push_back(std::make_pair(from.root() + "/" +std::get<0>(items[i]),to.get_absolute_path(std::get<2>(items[i]))) );
                if(size >= 1024*1024 || i == (items.size() -1)){ //超过1M分块批量上传
                    log::Info() << "attpemt to upload " << to_copy.size() <<" files in " << size << "B...";
                   
                    std::vector<return_info_t> result;
                    error_info_t e;
                    VERIFY_TRANSFER_RESULT(to.get_transfer().batch_upload_from_file(to_copy,false,result,e),e);
                    VERIFY_TRUE(to_copy.size() == result.size()) <<" request count != respond count";
                    for(size_t t = 0 ; t < result.size(); t++){
                        VERIFY_TRANSFER_RESULT(result[t].is_success || result[t].error_info.error_code == FILE_EXIST,result[t].error_info) ;
                        log::Debug() << "muti_copy " << to_copy[t].first << " -> " <<" cloud:" << to_copy[t].second;
                    }
                    size = 0;
                    to_copy.clear();
                }
            }
        }
        template<>
        void muti_copy(CloudStorage& from, FileStorage& to, const CopyItems & items){
            std::vector<std::pair<std::string,std::string> > to_copy;
            std::vector<std::string> to_path;
            size_t size = 0;
            boost::filesystem::create_directories(to.root() +"/.ezsync/temp/");
            bool all_succecss = true;
            for(size_t i=0; i< items.size(); i++){
                //如果文件在目标Storage中被排除,则不应该下载
                if(!to.include( std::get<2>(items[i]) )){
                    log::Warn() << "skip copy EXCEPT file" << "cloud:" <<from.root() << "/"<<std::get<0>(items[i]) << " -> " << to.root() << "/" << std::get<2>(items[i]) ;
                    return ;
                }else{
                    log::Debug() << "copy " << "cloud:" <<from.root() << "/"<<std::get<0>(items[i]) << " -> " << to.root() << "/" << std::get<2>(items[i]) << " size:"<< std::get<1>(items[i]).size;
                }
                //先下载到临时目录，在移动到目标目录
                size += std::get<1>(items[i]).size;
                std::string temp_file = to.root() + "/.ezsync/temp/" + get_text_md5(std::get<2>(items[i]));
                to_copy.push_back(std::make_pair(from.root()+"/"+std::get<0>(items[i]),temp_file));
                to_path.push_back(to.root() + "/" + std::get<2>(items[i]));
                if(size >= 1024*1024 || i == (items.size()-1)){ //超过1M分块批量下载
                    std::vector<return_info_t> result;
                    error_info_t e;
                    log::Info() << "attpemt to download " << to_copy.size() <<" files in " << size << "B...";
                    from.get_transfer().batch_download_to_file(to_copy, result,e);
                    //VERIFY_TRUE(to_copy.size() == result.size()) <<" request count != respond count";
                    for(size_t t = 0 ; t < result.size(); t++){
                        VERIFY_TRANSFER_RESULT(result[t].error_info.error_code != AUTH_FAILED,result[t].error_info);
                        if(!result[t].is_success) {                           
                            if(result[t].error_info.error_code != FILE_NOT_EXIST) all_succecss = false; // 忽略文件不存在的情况
                            log::Warn() << "muti_copy cloud: " 
                                << to_copy[t].first << " -> " << to_path[t]
                                << " failed "<<result[t].error_info.error_code << " "<< result[t].error_info.error_msg
                                << " " << e.error_code << " "<<e.error_msg;
                        }else{
                            log::Debug() << "muti_copy cloud: " 
                                << to_copy[t].first << " -> " << to_copy[t].second;

                            boost::filesystem::path path(to_path[t]);
                            boost::filesystem::remove(path);
                            boost::filesystem::create_directories(path.parent_path());
                            //TODO: 修改文件时间
                            //boost::filesystem::last_write_time(temp_file,src.modified_time);
                            boost::filesystem::rename(to_copy[t].second,path);
                        }
                        
                    }
                    size = 0;
                    to_path.clear();
                    to_copy.clear();
                }              
            }
            VERIFY_TRUE(all_succecss) <<"something wrong";

        }
    }
}

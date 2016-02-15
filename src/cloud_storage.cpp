/**
* $Id: cloud_storage.cpp 792 2014-04-13 15:23:43Z caoyangmin $ 
* @author caoyangmin(caoyangmin@gmail.com)
* @brief
*/
#include <sys/stat.h>
#include "ezsync/verify_true.h"
#include "ezsync/cloud_storage.h"
#include "ezsync/TransferInterface.h"
#include "ezsync/log.h"
#include "ezsync/verify_transfer_result.h"
using namespace ezsync;

CloudStorage::CloudStorage(std::shared_ptr<TransferInterface> transfer,const std::string& root)
    :_transfer(transfer)
    ,_root(root){
        if(!_root.empty() && _root[0] != '/') _root = "/" + _root; //必须以/开头
}
void CloudStorage::del(const std::string& key){
    error_info_t e;
    VERIFY_TRANSFER_RESULT(_transfer->del(get_absolute_path(key),e),e);
}

Entry CloudStorage::get_entry(const std::string& key)const{
    VERIFY_TRUE(false)<<"unimplemented";
    return Entry();
}

std::map<std::string,Entry> CloudStorage::get_all_entries(
    const std::string& path,
    unsigned int modify_time)const{
        VERIFY_TRUE(false)<<"unimplemented";
        return std::map<std::string,Entry>();
}

void CloudStorage::set_entry(const std::string&path, const std::string& from_path){
   error_info_t e;
   e.error_code =0;
   VERIFY_TRANSFER_RESULT(_transfer->upload_from_file(from_path,get_absolute_path(path),false,e) || e.error_code == FILE_EXIST,e);
   log::Debug()<<"upload "<<from_path << "->" << path<<" ok" << ( e.error_code == FILE_EXIST ? " exist,no overwrite":"");
}
std::string CloudStorage::get_absolute_path(const std::string&path)const{
    if(path.empty()) return _root;
    if(path[0] != '/')
        return _root.empty()?path:_root+"/"+path;
    else
        return _root.empty()?path:_root+path;
}
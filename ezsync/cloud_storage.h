/**
* $Id: cloud_storage.h 717 2014-03-26 03:39:28Z caoyangmin $ 
* @author caoyangmin(caoyangmin@gmail.com)
* @brief
*/
#pragma once
#include <memory>
#include "utility.h"
namespace ezsync{
    class TransferInterface;
    /** 
     * 云端存储
     */
    class CloudStorage
    {
    public:
        /**
         * @param root 根目录
         */
        CloudStorage(std::shared_ptr<TransferInterface> transfer,const std::string& root);
        
        const std::string& root()const{
            return _root;
        }
        /**
         * 删除条目
         * @param path 相对路径
         */
        void del(const std::string&path);
        /**
         * 获取条目(文件)信息
         * @param path 相对路径 TODO:urlencode
         */
        Entry get_entry(const std::string& path)const;
        /**
         * 获取指定目录下的所有条目(文件)信息
         * @param path 相对路径
         * @param modify_time 只获取此修改时间>=modify_time的条目
         */
        std::map<std::string,Entry> get_all_entries(
            const std::string& path,
            unsigned int modify_time)const;
        /**
         * 设置某一行的内容
         */
        void set_entry(const std::string&path, const std::string& from_path);

        TransferInterface& get_transfer(){
            return *_transfer;
        }
        std::string get_absolute_path(const std::string&path)const;
    private:
        
        std::string _root;
        std::shared_ptr<TransferInterface> _transfer;
        std::map<std::string,std::string> _request_head;
    };
}

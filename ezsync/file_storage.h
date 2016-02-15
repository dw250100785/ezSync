/**
* $Id: file_storage.h 986 2014-07-08 08:38:49Z caoyangmin $ 
* @author caoyangmin(caoyangmin@gmail.com)
* @brief
*/
#pragma once
#include "boost/filesystem.hpp"
#include "ezsync/utility.h"
#include "ezsync/path_clip_box.h"
namespace ezsync{
    /** 文件形式存储*/
    class FileStorage
    {
    public:
        /**
         * @param root 根目录
         */
        FileStorage(const std::string&root,const std::string &include = "*",const std::string &except = "");
        const std::string& root()const;
        /**
         * 创建新条目(文件)
         * @param key 相对路径
         */
       // void create_entry(const std::string&key);
        /**
         * 创建新文件
         * @param path 相对路径
         */
        void del(const std::string&path);
        /**
         * 获取条目(文件)信息
         * @param path 相对路径
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
        bool include(const std::string&path){
            return _path_clip.include(path);
        }
        //unsigned int get_last_modified_time();
        /**
         * 更新本地数据结构的版本
         */
        void upgrade(){}
    private:
        PathClipBox _path_clip;
        std::string get_absolute_path(const std::string&path)const;
        //获取相对路径
        //刚好能工作,不可靠的算法...
        std::string get_relative_path(const boost::filesystem::path& abs_path)const;
        Entry get_entry_by_path(const std::string& path)const;
        std::string _root;
    };
}

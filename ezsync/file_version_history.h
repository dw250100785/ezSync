/**
* $Id: file_version_history.h 628 2014-03-11 13:08:06Z caoyangmin $ 
* @author caoyangmin(caoyangmin@gmail.com)
* @brief ezsync::FileVersionHistory
*/
#pragma once
#include "boost/filesystem.hpp"
#include "boost/property_tree/ptree.hpp"
#include "utility.h"

namespace ezsync{
    /**
    * 文件方式存储的版本库
    * TODO: 如果版本库中条目数很多的话,可能要考虑用db存储版本信息
    */
    class FileVersionHistory
    {
    public:
        /**
        * @param root 根目录
        */
        FileVersionHistory(const std::string&root);
        /**
        * 获取总版本号
        */
        unsigned int get_version_num()const;
        /**
        * 获取单个项目的版本号
        */
        unsigned int get_version_num(const std::string& key);
        /**
        * 获取版本最后修改时间
        */
        unsigned int get_modified_time()const;
        /**
        * 升级到指定版本号
        */
        void upgrade(unsigned int to_version,const std::map<std::string,Entry>& changed);
        /**
        * 取某个条目的信息
        */
        VersionEntry get_entry(const std::string& key);
        /**
        * 获取指定目录下所有项目信息
        */
        std::map<std::string, VersionEntry> get_all_entries(const std::string& path);
        /**
        * 获取指定目录下,版本>version的所有项目信息
        */
        std::map<std::string, VersionEntry> get_changed_from(unsigned int version, const std::string& path);
        /**
        * 合并历史版本信息
        * 默认每次更新的版本信息都单独保存,便于比较两个版本之间的差异,但会降低比较版本号差距很大版本的效率
        * 所以可以把多个版本合并到同一个版本信息中,那么任何两个版本的差异都可以在此数据中获取到,但每次加载的数据量会变大
        */
        void discard_history();
        //删除标记相关
        void add_delete_mark(const std::string& key);
        bool has_delete_mark(const std::string& key,const VersionEntry&ori);
        void clear_delete_marks();
        std::map<std::string, VersionEntry> get_delete_marks();
        //修改时间相关
        void set_custom_value(const std::string& key,const std::string& value);
        std::string get_custom_value(const std::string& key);

        //预处理文件名,便于作为xml\json等格式的key
        boost::property_tree::ptree::path_type filter_key(const std::string& key)const ;
        boost::filesystem::path get_current_version_file()const;
        unsigned int get_from_version(const boost::property_tree::ptree& doc)const;
        void reload();

        const std::string&root()const{
            return _root;
        }
    private:
        void get_map_from_tree(std::map<std::string, VersionEntry>& map, 
            const boost::property_tree::ptree& doc,
            unsigned int from_version,const std::string&recursion="")const;
        
        bool append_next_history(boost::property_tree::ptree& doc);
        void merge_to(const boost::property_tree::ptree& child, boost::property_tree::ptree& merged) const;
        bool is_leaf(const boost::property_tree::ptree& child)const;

        std::string get_absolute_path(const std::string&path)const;
        std::string _root;
        boost::filesystem::path _current_version;
        boost::property_tree::ptree _doc;
    };
}
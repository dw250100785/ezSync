/**
* $Id: cloud_version_history2.h 1011 2014-08-05 06:45:25Z caoyangmin $ 
* @author caoyangmin(caoyangmin@gmail.com)
* @brief
*/
#pragma once
#include <set>
#include <tuple>
#include <queue> 
#include "boost/filesystem.hpp"
#include "boost/property_tree/ptree.hpp"
#include "ezsync/utility.h"

//TODO: 写的太急,需要重构
//TODO: 每个版本一个文件,当版本较多时,将影响性能
namespace ezsync{
    class TransferInterface;
    
    /**
     * 云端存储的版本库
	 * 本地以文件的形式保存每个版本与上一版本的变化信息，每个版本一个文件
	 * 目前更新只能以版本为单位，且只能顺序更新，不能随机更新
     */
    class CloudVersionHistory2
    {
    public:
		/**
	     * update 方法的回调，用于获取当前需要更新的项目，和返回已经更新的项目
		 */
        typedef std::function<bool(const std::map<std::string, VersionEntry>&, std::list<std::string>&)> UpdateRout;
        typedef std::map<std::string,Entry> CommitItems;
        /**
         */
        CloudVersionHistory2(std::shared_ptr<TransferInterface> transfer,const std::string&local_root,const std::string&remote_root);
        /**
         * @param rout,@see UpdateRout 回调,如果返回true,则这次回调中包含的版本下载完成;返回false,则结束当前及之后的版本下载
         * 每次update可能会回调若干次rout,且最后一次rout(empty)回调表示全部完成。
         * 如果rout()抛出异常,这次update中下载的所有版本均视为无效。
         */
        void update(UpdateRout rout);
		/** 递交版本修改*/
        unsigned int commit(const CommitItems&);
		/**
         * 标记需递交的项目
         * 标记过的项目将被记录修改时间,下次递交时将使用标记的修改时间,而不是递交的时间
         */
        void sign_commit(const CommitItems&);
        //修改时间相关
        void set_custom_value(const std::string& key,const std::string& value);
        std::string get_custom_value(const std::string& key);
        VersionEntry get_entry(const std::string& key)const;//获取当前某一项的本地版本,可能不是最新的
        VersionEntry get_latest_entry(const std::string& key)const;//获取当前某一项的最新版本,可能比本地的新
        //预处理文件名,便于作为xml\json等格式的key
        boost::property_tree::ptree::path_type filter_key(const std::string& key)const ;
        /** 重新加载版本信息*/
		void reload();
		/** 获取最新的版本信息*/
        void refresh();
        void add_delete_mark(const std::string& key);
        bool has_delete_mark(const std::string& key,const VersionEntry&ori);
        void clear_delete_marks();
        std::map<std::string, VersionEntry> get_delete_marks();
        const std::string& local_root()const{
            return _local_root;
        }
    private:
        static const size_t s_max_download_count = 100;//批量下载时,每次最多下载的版本文件数
        //boost::filesystem::path get_current_version_file()const;
        //下载指定的版本文件到临时目录
        //如果版本不存在,返回false,且before_version会被指为前一个最近存在的版本
        bool download(const std::string&  version,std::string&  before_version);
        void get_map_from_tree(std::map<std::string, VersionEntry>& map, 
            const boost::property_tree::ptree& doc,
            const std::string&recursion="")const;
        std::string get_from_version(const boost::property_tree::ptree& doc)const;
        std::string get_from_version(const std::string&)const;
        bool append_next_history(boost::property_tree::ptree& doc);
        typedef std::function<void( unsigned int,const std::string&)> PreOverwrite;
        void merge_to(const boost::property_tree::ptree& child, boost::property_tree::ptree& merged, 
            const PreOverwrite& pre_overwrite=PreOverwrite() ,const std::string &recursion = "") const;
        bool is_leaf(const boost::property_tree::ptree& child)const;
        //std::map<std::string, VersionEntry> _cached_changed;
        std::map<unsigned int, std::string> _updated_version;
        static std::tuple<unsigned int,std::string> split_version_str(const std::string& version);
        std::string get_local_path(const std::string&path)const;
        std::string get_remote_path(const std::string&path)const;
        std::string _local_root;
        std::string _remote_root;
        std::string _current_cursor;//最后一个更新的版本
        std::string _remote_version;//当前云端版本
        std::shared_ptr<TransferInterface> _transfer;
        bool is_updated(unsigned int)const;
        bool is_updated(const std::string&)const;
        void valid_version(const std::string& version);
        //void record_latest_version(const std::string& version);
        boost::property_tree::ptree _doc;
        std::map<unsigned int,std::string> _history_versions;//历史版本以及对应的下载路径
        void refresh_history_version_list();
        std::list<std::string> _nil_versions;
        typedef std::map<unsigned int,std::pair<std::string,std::map<std::string, VersionEntry> > > VersionDetail;
        VersionDetail _version_detail;//版本明细
        boost::property_tree::ptree _sign_commit;//递交标记
        boost::property_tree::ptree _latest;//最新
        std::map<std::string, std::shared_ptr<boost::property_tree::ptree> > _temp_cache;
    };
    typedef CloudVersionHistory2 CloudVersionHistory;
}

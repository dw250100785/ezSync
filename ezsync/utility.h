/**
* $Id: utility.h 792 2014-04-13 15:23:43Z caoyangmin $ 
* @author caoyangmin(caoyangmin@gmail.com)
* @brief
*/
#pragma once

#include <map>
#include <string>
#include <list>

namespace ezsync{
    /**
     * 计算文本串的MD5
     */
    std::string get_text_md5(const std::string& text);
    /**
     * 计算文件的MD5
     */
    std::string get_file_md5(const std::string& path);
    struct Entry{
        Entry()
            :modified_time(0)
            ,size(0)
        {
        }
        unsigned int modified_time; //<! 修改时间
        std::string md5;            //<! TODO: 只在必要的时候才计算MD5
        std::string content;        //内容
        size_t size;                //<! 大小
        bool is_exist()const{       //<! 项目是否存在
            bool t1 = !content.empty();
            bool t2 = !md5.empty();
            return t1 || t2; 
            //return /*!storage_path.empty() ||*/ !content.empty() || !md5.empty();
        }
    };
    enum UseWitch {
        USE_LOCAL=0,
        USE_REMOTE,
        USE_UNKNOWN=-1
    };
    struct VersionEntry:Entry{
        VersionEntry():version(0){
        }
        unsigned int version;
    };
    enum SyncEntryOperator {
        UNCHANGED_OP = 0,
        NEW_OP = 1,
        MODIFY_OP= 2,
        DELETE_OP= 3,
        UNKNOWN_OP = -1,
    };
    struct ChangedEntrie{
        ChangedEntrie()
        :op(UNKNOWN_OP){
        }
        ChangedEntrie(const Entry& e,SyncEntryOperator o)
        :entry(e)
        ,op(o){
        }
        Entry entry;
        SyncEntryOperator op;
    };
    typedef std::map<std::string,ChangedEntrie> ChangedEntries;

    //自定义编码:初字母数字外的所有字符转成 _+十六进制
    std::string encode_key(const std::string& key,char sap='_');
    std::string decode_key(const std::string& key,char sap='_');

    /** 本地与历史差异 <历史项目,当前项目 >*/
    typedef std::pair<VersionEntry,Entry> LocalDiff;
    /** 本地与历史差异集合*/
    typedef std::map<std::string,LocalDiff> LocalDiffs;
    /** 本地与远端差异 <本地当前项目,远端当前项目 >*/
    typedef std::pair<VersionEntry,VersionEntry> RemoteDiff;
    /** 本地与远端差异集合 */
    typedef std::map<std::string,RemoteDiff> RemoteDiffs;
    /**配置信息*/
    typedef std::map<std::string,std::string> Config;
    
}

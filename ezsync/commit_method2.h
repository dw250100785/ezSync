/**
* $Id: commit_method2.h 1011 2014-08-05 06:45:25Z caoyangmin $ 
* @author caoyangmin(caoyangmin@gmail.com)
* @brief CommitMethod2
*/
#pragma once
#include <string>
#include "boost/algorithm/string.hpp"
#include "boost/filesystem.hpp"
#include "ezsync/verify_true.h"
#include "ezsync/utility.h"
#include "ezsync/log.h"
#include "ezsync/path_clip_box.h"
#include "ezsync/transfer.h"

namespace ezsync{
    /** 需要先执行update*/
    class RequestUpdateException:public std::exception{};
    /**
     * 递交修改
     * 扫描本地修改项目,并同步到远端
     * 先上传文件,文件名后加MD5防止覆盖老版本
     */
    class CommitMethod2
    {
    public:
        /**
         * @param path 需要递交的路径,""即全部更新
         * TODO: 增加参数,控制是否把缺失的项目当作删除递交
         */
        CommitMethod2(const std::string& path="",bool sign_only=false)
            :_path(path)
            ,_clip_box(path)
            ,_total_commit(path.empty())
            ,_sign_only(sign_only)
            //,_commit_lost(commit_lost)
        {
             //普通路径
                if((path.find("*") == std::string::npos) && (path.find("\n") == std::string::npos)){
                   _clip_box = PathClipBox();
                }else{
                    _path = "";
                }
        }
        const ChangedEntries& get_changed()const {
            return _changed;
        }
        template<class Client>
        void execute(Client& client){
            log::Info()<<"attempt to commit:" << client.get_local_storage().root() << "<->"<<client.get_remote_storage().root();

            _changed.clear();
            LocalDiffs diffs = get_local_diff(client);
            if(diffs.empty()) {
                log::Info()<<"commit ok without changed";
                return ; //本地无修改,则无需递交
            }
            //要求递交时版本与远端的一致,可以防止多个设备对同一条记录做的修改互相覆盖,因为update时能检查到冲突。
            //为了解决版本差异较大时,必须下载全部差异,才能递交,导致递交时间过长的问题,所以取消了对递交的这一限
            //制。其副作用是,如果递交时未进行update(或者只update了一部分),那么将无法检查到冲突(或者只检查到update
            //的这部分项目的冲突)。TODO: 如果要解决这一副作用,可以使多分支版本管理的办法,递交时上传到分支,update时
            //对分支进行合并,如下:
            // A 设备  0->1-----+-->3(update,合并1,2,然后commit)
            // B 设备  0---->2--+-------+--->5(update,合并2,4,然后commit)
            // C 设备  0----------->4---+
           
            //开始同步修改项目
            std::map<std::string,Entry> changed;
            transfer::CopyItems to_upload;
            to_upload.reserve(diffs.size());
            ChangedEntries temp;
            for(LocalDiffs::const_iterator it = diffs.begin(); it != diffs.end(); it++){
                changed[it->first] = it->second.second;
                if(!it->second.second.is_exist() ){
                    //删除,TODO: 记录哪些项目被删除,最后commit完成后,需要删除远端的这些项目
                    temp[it->first]=ChangedEntrie(it->second.second,DELETE_OP);
                    log::Debug() << it->first << " deleted";
                }else{
                    //修改
                    if(!it->second.first.is_exist()){
                        temp[it->first]=ChangedEntrie(it->second.second,NEW_OP);
                        log::Debug() << it->first << " new";
                    }else{
                        temp[it->first]=ChangedEntrie(it->second.second,MODIFY_OP);
                        log::Debug() << it->first << " modified";
                    }
                    /*if(client.get_local_storage().get_entry(it->first).md5 == client.get_remote_version().get_entry(it->first).md5){
                        log::Debug()<<it->first<<" skip upload,no diff";
                    }else*/{
                        to_upload.push_back(std::make_tuple(
                            it->first,it->second.second,
                            it->first+"$"+it->second.second.md5));// TODO: 是否应该增加版本号?
                        
                    }
                }
            }
            client.upload(to_upload); 
            if(_sign_only){
                client.get_version().sign_commit(changed);
                log::Info() << "sign commit ok";
            }else{
                //更新版本库信息
                unsigned int ver = client.get_version().commit(changed);

                _changed = temp;
                log::Info() << "commit ok version:" << ver;
            }
        }
    private:
        bool _total_commit;// 全部递交
        PathClipBox _clip_box;
        ChangedEntries _changed;
        /** 获取远端与本地快照的diff */
        template<class Client>
        LocalDiffs get_local_diff(Client& client){
            LocalDiffs diff;
            //先根据修改时间,获取最后一次递交后修改过的项目
            //这些项目可能包含update的项目
            std::map<std::string,Entry> changed = client.get_local_storage().get_all_entries(_path,0);
            for(std::map<std::string,Entry>::iterator it = changed.begin(); it != changed.end(); it++){
                if(!_clip_box.include(it->first)){
                    log::Debug()<< it->first << " get_local_diff skip";
                    continue;
                }
                VersionEntry ori = client.get_version().get_latest_entry(it->first);
                if(!ori.is_exist()){
                    diff[it->first] = std::make_pair(ori, it->second);
                }else /*if(it->second.modified_time >= ori.modified_time)*/{ //比较修改时间可以加快速度,但可靠性降低
                    if(ori.size != it->second.size || ori.md5 != it->second.md5){
                        diff[it->first] = std::make_pair(ori, it->second);
                    }
                }
            }
            //扫描被删除的项目
            std::map<std::string,VersionEntry> deleted = client.get_version().get_delete_marks();
            for(std::map<std::string,VersionEntry>::iterator it = deleted.begin(); it != deleted.end(); it++){
                diff.insert(std::make_pair(it->first, std::make_pair(it->second, Entry())));
            }
            return diff;
        }
        std::string _path;
        bool _sign_only;
        //bool _commit_lost;
    };
    typedef CommitMethod2 CommitMethod;
}

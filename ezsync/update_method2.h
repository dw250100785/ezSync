/**
* $Id: update_method2.h 1011 2014-08-05 06:45:25Z caoyangmin $ 
* @author caoyangmin(caoyangmin@gmail.com)
* @brief
*/
#pragma once

#include <functional>
#include <list>
#include <set>
#include <string>
#include <boost/algorithm/string.hpp>
#include "verify_true.h"
#include "utility.h"
#include "ezsync/log.h"
#include "ezsync/path_clip_box.h"
#include "ezsync/transfer.h"
namespace ezsync{

    /**
    * 冲突合并策略
    * UseWitch (const std::string& key, Entry& local, Entry& remote)
    * @return bool 返回true合并成功,false合并失败
    * @param key 合并项目的key,如文件名、数据库主键等
    * @param local
    * @param remote
    */
    typedef std::function<UseWitch(const std::string&,const Entry&,const Entry&)> MergeStrategy;

    typedef std::function<bool (const ChangedEntries&, bool &limited,std::list<std::string>& limit )> PreUpdate;
    typedef std::function<bool (const ChangedEntries&)> PostUpdate;
    /**
    * 更新方法
    * 获取最新的数据,从远端 -> 本地
    * 与UpdateMethod相比，主要是增加可以分多次下载更新的功能。
    */
    class UpdateMethod2 {
    public:
        UpdateMethod2(MergeStrategy strategy,const std::string&include,const std::string&except,PostUpdate post_update=PostUpdate())
            :_merge_strategy(strategy)
            ,_init(false)
            ,_clip(include,except)
            ,_post_update(post_update){
                log::Debug() <<"UpdateMethod2: include "<<include << "\nexcept " << except;
                std::list<std::string> items;
                boost::algorithm::split( items, include, boost::is_any_of("\n") );
                bool  fuzzy_matching= false;
                std::for_each(items.begin(),items.end(),[&fuzzy_matching,this](const std::string& i){
                    if(!fuzzy_matching){
                        if(i.find("*") != std::string::npos){ // 如果包含通配符*,则只能全部更新
                            fuzzy_matching = true;
                            this->_pickout.clear();
                        }else if(!i.empty()){
                            this->_pickout.insert(i);
                        }
                    }
                });

        }
        UpdateMethod2(MergeStrategy strategy,PreUpdate pre_update,PostUpdate post_update)
            :_merge_strategy(strategy)
            ,_init(false)
            ,_pre_update(pre_update)
            ,_post_update(post_update){

        }
        template<class Client>
        void init(Client& client){
            log::Info() <<"attempt to update: "<<client.get_local_storage().root() << "<->" << client.get_remote_storage().root();
            client.get_version().refresh();
            _changed.clear();
            _changed_temp.clear();
        }
        bool _init;
        transfer::CopyItems _to_download;

        //从新到旧,逐个版本进来
        template<class Client>
        bool pickout_rout(Client&client,std::set<std::string>&pickout,const std::map<std::string, VersionEntry>& diff,std::list<std::string>& completed){
            //只更新指定的项目
            if(diff.empty()){//所有指定的项目均找到，完成
                //开始下载
                if(!_to_download.empty()){
                    client.download(_to_download);
                    _to_download.clear();
                }
                _changed = _changed_temp;
                std::for_each(_changed_temp.begin(),_changed_temp.end(),[&completed](ChangedEntries::reference it){
                    completed.push_back(it.first);
                });
                if(_post_update){
                    VERIFY_TRUE(_post_update(_changed))<<"post update failed";
                }
                return true;
            }
            for(std::set<std::string>::iterator it = pickout.begin(); it != pickout.end(); ){
                std::set<std::string>::iterator temp = it++;
                VersionEntry entry = client.get_version().get_latest_entry(*temp);
                if(entry.is_exist()){ //如果项目已存在, 一旦确定当前版本是最新版本,即可以停止扫描
                    if(entry.version >= diff.begin()->second.version){
                        //检查冲突
                        Entry local = client.get_local_storage().get_entry(*temp);
                        if((local.is_exist() && (local.size != entry.size || local.md5 != entry.md5 ))){
                            UseWitch use = _merge_strategy(*temp,local,entry);
                            VERIFY_TRUE(use == USE_LOCAL || use == USE_REMOTE)<< *temp << " merge failed "<<use;
                            if(use == USE_LOCAL){
                                log::Debug() << *temp  << " conflict resolved:use local";
                                _changed_temp[*temp]=ChangedEntrie(entry,UNCHANGED_OP);
                            }else if(use == USE_REMOTE){
                                std::ostringstream from;
                                from << *temp <<"$"<< entry.md5;
                                _to_download.push_back(std::make_tuple(from.str(),entry,*temp));
                                log::Debug() << *temp  << " conflict resolved:use remote";
                                _changed_temp[*temp]=ChangedEntrie(entry,MODIFY_OP);
                            }else{
                                VERIFY_TRUE(false)<<"don\'t known how to merge!";
                            }
                            
                        }else if(!local.is_exist()){
                            std::ostringstream from;
                            from << *temp <<"$"<< entry.md5;
                            log::Debug() << *temp  << " recover";
                            _to_download.push_back(std::make_tuple(from.str(),entry,*temp));
                            _changed_temp[*temp]=ChangedEntrie(entry,MODIFY_OP);
                        }
                        log::Debug()<<*temp << " pickout break @ " << diff.begin()->second.version;
                        pickout.erase(*temp);
                    }
                }
            }
            if(pickout.empty()){
                return false; //结束
            }else{
                return true;
            }
        }
        template<class Client>
        bool update_rout(Client&client,const std::map<std::string, VersionEntry>& diff,std::list<std::string>& completed){
            if(diff.empty()){
                if(!_to_download.empty()){
                    client.download(_to_download);
                    _to_download.clear();
                }
                _changed = _changed_temp;
                std::for_each(_changed_temp.begin(),_changed_temp.end(),[&completed](ChangedEntries::reference it){
                    completed.push_back(it.first);
                });
                if(_post_update){
                    VERIFY_TRUE(_post_update(_changed))<<"post update failed";
                }
                return true;
            }
            RemoteDiffs remote_diff = get_remote_diff(client,diff);
            ChangedEntries changed = merge_diff(client,remote_diff);
            if(changed.empty()) return true;
            bool limited= false;
            std::list<std::string> todo;
            bool to_continue = true;
            if(_pre_update){
                to_continue = _pre_update(changed,limited,todo);
            }
            if(limited){
                for(std::list<std::string>::iterator it = todo.begin();
                    it != todo.end();
                    it++){
                        ChangedEntries::iterator diff = changed.find(*it);
                        VERIFY_TRUE( diff != changed.end()) << "unknown " << *it;
                        if(diff->second.op == DELETE_OP){
                            client.get_local_storage().del(diff->first);
                            log::Debug() << diff->first  << " deleted";
                        }else if(diff->second.op == NEW_OP || diff->second.op == MODIFY_OP){
                            std::ostringstream from;
                            from << diff->first<<"$"<< diff->second.entry.md5; // TODO: 后缀需要再加上版本号吗?
                            _to_download.push_back(std::make_tuple(from.str(),diff->second.entry,diff->first));
                        }else{
                            VERIFY_TRUE(false) <<"unknown operator "<< diff->first <<" "<<diff->second.op;
                        }
                        _changed_temp[diff->first]=diff->second;
                        changed.erase(diff);
                }
            }else{
                for(ChangedEntries::iterator it = changed.begin();
                    it != changed.end();
                    it++){
                        if(it->second.op == DELETE_OP){
                            client.get_local_storage().del(it->first);
                            log::Debug() << it->first  << " deleted";
                        }else if(it->second.op == NEW_OP || it->second.op == MODIFY_OP){
                            std::ostringstream from;
                            from << it->first<<"$"<< it->second.entry.md5; // TODO: 后缀需要再加上版本号吗?
                            _to_download.push_back(std::make_tuple(from.str(),it->second.entry,it->first));
                        }else if(UNCHANGED_OP == it->second.op){
                            log::Debug() << it->first  << " unchanged";
                        }else{
                            VERIFY_TRUE(false) <<"unknown operator "<< it->first <<" "<<it->second.op;
                        }
                        _changed_temp[it->first]=it->second;
                }
                changed.clear();
            }
            if(!changed.empty()){ //未全部更新
                log::Debug() << "has more...";
            }
            return to_continue;
        }
        template<class Client>
        void execute(Client& client){
          
            _changed.clear();
            _changed_temp.clear();
            if(!_init){
                init(client);
                _init = true;
            }
            _to_download.clear();
            if(!_pickout.empty()){//只更新指定的项目

                //逐个扫描最新的版本,直到找到所有项目的最新版本,然后逐个下载
                std::set<std::string> pickout = _pickout;
                client.get_version().update(std::bind(
					&UpdateMethod2::pickout_rout<Client>,
					this,client,pickout,std::placeholders::_1,std::placeholders::_2));
                transfer::CopyItems to_download;
                for(std::set<std::string>::iterator it = pickout.begin(); it != pickout.end(); it++){
                    VersionEntry entry = client.get_version().get_latest_entry(*it);
                    if(!entry.is_exist()){
                        log::Warn()<< *it << " not exist @ remote";
                    }else{
                        Entry local = client.get_local_storage().get_entry(*it);
                        if((local.is_exist() && (local.size != entry.size || local.md5 != entry.md5 ))){
                            UseWitch use = _merge_strategy(*it,local,entry);
                            VERIFY_TRUE(use == USE_LOCAL || use == USE_REMOTE)<< *it << " merge failed "<<use;
                            if(use == USE_LOCAL){
                                log::Debug() << *it  << " conflict resolved:use local";
                                _changed_temp[*it]=ChangedEntrie(entry,UNKNOWN_OP);
                            }else if(use == USE_REMOTE){
                                std::ostringstream from;
                                from << *it <<"$"<< entry.md5;
                                to_download.push_back(std::make_tuple(from.str(),entry,*it));
                                log::Debug() << *it  << " conflict resolved:use remote";
                                _changed_temp[*it]=ChangedEntrie(entry,MODIFY_OP);
                            }else{
                                VERIFY_TRUE(false)<<"don\'t known how to merge!";
                            }

                        }else if(!local.is_exist()){
                            std::ostringstream from;
                            from << *it <<"$"<< entry.md5;
                            log::Debug() << *it  << " recover";
                            to_download.push_back(std::make_tuple(from.str(),entry,*it));
                            _changed_temp[*it]=ChangedEntrie(entry,MODIFY_OP);
                        }
                    }
                    client.download(to_download);
                }
                log::Info() <<"update ok (pickout)";
            }
            else{//全部更新
                client.get_version().update(std::bind(&UpdateMethod2::update_rout<Client>,this,client,std::placeholders::_1,std::placeholders::_2));
                log::Info() <<"update ok";
            }
        
            
        }
        const ChangedEntries& get_changed()const{
            return _changed;
        }
    private:
        std::set<std::string> _pickout; //指定只跟新这些项目
        PostUpdate _post_update;
        MergeStrategy _merge_strategy;
        ChangedEntries _changed;
        ChangedEntries _changed_temp;
        PreUpdate _pre_update;
        PathClipBox _clip;
        /** 获取远端与本地快照的diff */
        template<class Client>
        RemoteDiffs get_remote_diff(Client& client,const std::map<std::string,VersionEntry>&changed ){
            RemoteDiffs diff;
            for(std::map<std::string,VersionEntry>::const_iterator it = changed.begin(); it != changed.end(); it++){
                if(!_clip.include(it->first)){
                    log::Debug()<< it->first << " get_remote_diff skip";
                    continue;
                }
                diff[it->first] = std::make_pair(client.get_version().get_entry(it->first), it->second);
            }
            return diff;
        }

        /**
        * 合并修改,
        * 判断远端修改的项目,本地是否也做了修改.如果是,则需要合并冲突
        */
        template<class Client>
        ChangedEntries merge_diff(Client& client,const RemoteDiffs& remote_diff){
            ChangedEntries changed;
            for(RemoteDiffs::const_iterator it = remote_diff.begin(); it != remote_diff.end(); it++){
                VersionEntry local_ori = client.get_version().get_entry(it->first);
                Entry local_cur = client.get_local_storage().get_entry(it->first);
                if(!local_cur.is_exist()){//文件不存在,确定是否是标记为删除(未标记,说明只是本地删除,但远端未删除,此时当前版本设置成=为删除版本)
                    if(!client.get_version().has_delete_mark(it->first,local_ori)){
                        local_cur = local_ori;
                        log::Debug() << it->first  << " deleted,witout mark";
                    }else{
                        log::Debug() << it->first  << " deleted,wait commit";
                    }
                }
                if(local_ori.size != local_cur.size || local_ori.md5 != local_cur.md5){//本地做过修改,冲突
                    VersionEntry remote_cur = it->second.second;
                    log::Debug() << it->first  << " conflict:"<<local_ori.version<<" vs "<<remote_cur.version;
                    if(local_cur.size == remote_cur.size && local_cur.md5 == remote_cur.md5){
                        //修改后的版本与远端一致,什么也不用做
                        log::Debug() << it->first  << " conflict resolved:no diff,use local";
                        changed[it->first] = ChangedEntrie(remote_cur,UNCHANGED_OP);
                        continue;
                    }else{
                        UseWitch use = _merge_strategy(it->first,local_cur,remote_cur);
                        VERIFY_TRUE(use == USE_LOCAL || use == USE_REMOTE)<< it->first << " merge failed "<<use;
                        if(use == USE_LOCAL){
                            log::Debug() << it->first  << " conflict resolved:use local";
                            //如果选择使用本地，则项目的修改时间不会变化，为了能够在下次递交的时候找到该项目（递交时会考虑修改时间），需要记录这些项目。
                            //do nothing,just skip
                            changed[it->first] = ChangedEntrie(local_cur,UNCHANGED_OP);
                            continue;
                        }else if(use == USE_REMOTE){
                            log::Debug() << it->first  << " conflict resolved:use remote";
                            if(!it->second.second.is_exist() ){
                                changed[it->first] = ChangedEntrie(remote_cur,DELETE_OP);
                            }else{
                                changed[it->first] = ChangedEntrie(remote_cur,MODIFY_OP);
                            }
                            
                            continue;
                        }else{
                            VERIFY_TRUE(false)<<"don\'t known how to merge!";
                        }
                    }
                }
                //本地未做修改,使用远端的数据
                if(!it->second.second.is_exist() ){
                    changed.insert(std::make_pair(it->first,ChangedEntrie(it->second.second,DELETE_OP)));
                }else{
                    if(!it->second.first.is_exist()){
                        changed.insert(std::make_pair(it->first, ChangedEntrie(it->second.second,NEW_OP)));
                    }else{
                        changed.insert(std::make_pair(it->first, ChangedEntrie(it->second.second,MODIFY_OP)));
                    }
                }
            }
            return changed;
        }
    };
    typedef UpdateMethod2 UpdateMethod;
}

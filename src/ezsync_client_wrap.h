/**
* $Id: ezsync_client_wrap.h 986 2014-07-08 08:38:49Z caoyangmin $ 
* @author caoyangmin(caoyangmin@gmail.com)
* @brief 
*/
#pragma once
#include <functional>
#include <list>
#include "ezsync/utility.h"
#include "ezsync/single_access_lock.h"
#include "ezsync/TransferInterface.h"
#include "ezsync/update_method2.h"
#include "ezsync/delete_method2.h"
#include "ezsync/commit_method2.h"
#include "ezsync/upgrade_method.h"
class IEzsyncMergeStrategy{
public:
    virtual ~IEzsyncMergeStrategy(){}
    virtual ezsync::UseWitch merge(const std::string&,const ezsync::Entry&,const ezsync::Entry&) = 0;
};

class IUpdateListener{
public:
    virtual ~IUpdateListener(){}
    virtual bool post_update(const ezsync::ChangedEntries&) = 0;
};

class IEzsyncClientWrap{
public:
    virtual ~IEzsyncClientWrap(){}
    virtual ezsync::ChangedEntries update(IEzsyncMergeStrategy &strategy,const std::string&include, const std::string&except,IUpdateListener*listener)=0;
    virtual void del(const std::string& path)=0;
    virtual ezsync::ChangedEntries commit(const std::string& path)=0;
    virtual void upgradeLocalStorage()=0;
    virtual void cancel()=0;
};

template<class T>
class EzsyncClientWrap :public IEzsyncClientWrap{
public:
    EzsyncClientWrap(T*t)
    :_client(t){
        
    }

    bool post_update(IUpdateListener*listener,const ezsync::ChangedEntries& changed){
    	if(!listener) return true;
    	return listener->post_update(changed);
    }
    ezsync::ChangedEntries update(IEzsyncMergeStrategy &strategy,const std::string&include, const std::string&except,IUpdateListener*listener){
       	ezsync::UpdateMethod m(
       			std::bind(&IEzsyncMergeStrategy::merge,&strategy,std::placeholders::_1,std::placeholders::_2,std::placeholders::_3),
       			include,
       			except,
       			std::bind(&EzsyncClientWrap<T>::post_update, this,listener, std::placeholders::_1));
       	ezsync::SigleAccessLock lock(_client->get_version().local_root());
           _client->execute(m);
           return m.get_changed();
    }
    void del(const std::string& path){
    	ezsync::SigleAccessLock lock(_client->get_version().local_root());
    	ezsync::DeleteMethod m(path);
        _client->execute(m);
    }
    ezsync::ChangedEntries commit(const std::string& path){
    	ezsync::SigleAccessLock lock(_client->get_version().local_root());
    	ezsync::CommitMethod m(path);
        _client->execute(m);
        return m.get_changed();
    }
    void cancel(){
    	_client->cancel();
    }
    virtual void upgradeLocalStorage(){
    	ezsync::SigleAccessLock lock(_client->get_version().local_root());
    	ezsync::UpgradeMethod m;
    	_client->execute(m);
    }
private:
    std::shared_ptr<T> _client;
};

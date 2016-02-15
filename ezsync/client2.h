/**
* $Id: client2.h 757 2014-04-08 06:49:22Z caoyangmin $ 
* @author caoyangmin(caoyangmin@gmail.com)
* @brief ezsync::Client
*/

//TODO: 增加并发检测,禁止并发执行
#pragma once
#include <memory>
#include "ezsync/utility.h"
#include "ezsync/transfer.h"
namespace ezsync{
    /**
     * ezsync 客户端
     * 
     * Client<...> client;
     * Method method;
     * client.execute(method);
     */
    template<class LocalStorage_, //本地存储
    class RemoteStorage_,           //远端存储
    class VersionHistory_,          //远端版本控制
    class Controler_                //控制器,用随时取消正在进行的操作
    >
    class Client2{
    public:
        typedef  LocalStorage_ LocalStorage;
        typedef  RemoteStorage_ RemoteStorage;
        typedef  VersionHistory_ VersionHistory;
        typedef  Controler_ Controler;

        Client2(std::shared_ptr<LocalStorage> local_storage,
            std::shared_ptr<RemoteStorage> remote_storage,
            std::shared_ptr<VersionHistory> version_history,
            std::shared_ptr<Controler> controler)
        :_local_storage(local_storage)
        ,_remote_storage(remote_storage)
        ,_version_history(version_history)
        ,_controler(controler){
        }
        virtual ~Client2(){
        }
        /** 入口方法 */
        template<class T>
        void execute(T&t){
            if(_controler)_controler->reset();
            t.execute(*this);
        }
        virtual LocalStorage& get_local_storage(){
            return *_local_storage;
        }
        virtual RemoteStorage& get_remote_storage(){
            return *_remote_storage;
        }
        virtual VersionHistory& get_version(){
            return *_version_history;
        }
       /* virtual Transfer& get_transfer(){
            return *_transfer;
        }*/
        /**
        * 上传
        * 从local_storage 复制到 remote_storage
        */
        virtual void upload(const std::string& from_path,const Entry& src,const std::string& to_path){
            transfer::copy(get_local_storage(),from_path,src,get_remote_storage(),to_path);
            //std::cout<<"upload "<< from_path << " -> "<<to_path << std::endl;
        }
        /**
        * 下载
        * 从remote_storage 复制到 local_storage
        */
        virtual void download(const std::string& from_path,const Entry& src,const std::string& to_path){
            transfer::copy(get_remote_storage(),from_path,src,get_local_storage(),to_path);
            //std::cout<<"download "<< from_path << " -> "<<to_path << std::endl;
        }
        /*
        * 上传多个
        * 从local_storage 复制到 remote_storage
        */
        virtual void upload(const transfer::CopyItems& items){
            if(items.empty())return ;
            transfer::muti_copy(get_local_storage(),get_remote_storage(),items);
        }
        /**
        * 下载多个
        * 从remote_storage 复制到 local_storage
        */
        virtual void download(const transfer::CopyItems& items){
            if(items.empty())return ;
            transfer::muti_copy(get_remote_storage(),get_local_storage(),items);
        }

        /**
        * 取消正在执行的操作
        * 只对当前正在执行的操作有效
        */
        virtual void cancel(){
            if(_controler)_controler->cancel();
        }
    private:
        std::shared_ptr<Controler>_controler;
        //std::shared_ptr<Transfer> _transfer;
        std::shared_ptr<LocalStorage> _local_storage;
        std::shared_ptr<RemoteStorage> _remote_storage;
        std::shared_ptr<VersionHistory> _version_history;
    };
}

/**
* $Id: delete_method2.h 1011 2014-08-05 06:45:25Z caoyangmin $ 
* @author caoyangmin(caoyangmin@gmail.com)
* @brief
*/
#pragma once
#include "utility.h"

namespace ezsync{
    /**
     * 删除方法
     */
    class DeleteMethod2
    {
    public:
        DeleteMethod2(const std::string& key)
            :_key(key){
        }
        /**
         * 
         */
        template<class Client>
        void execute(Client& client){
            //TODO: 对目录的删除效率低
            log::Info()<<"attempt to delete " << _key;
            //先记录删除操作,然后删除实际文件
            std::map<std::string,Entry> entries = client.get_local_storage().get_all_entries(_key,0);
            for(std::map<std::string,Entry>::iterator it = entries.begin();
                it != entries.end();
                it++){
                    //VersionEntry ori = client.get_local_version().get_entry(it->first);
                    //if(ori.is_exist()){ //如果已经在版本中,需要标记为删除
                        client.get_version().add_delete_mark(it->first);
                    //}
                    client.get_local_storage().del(it->first);  
            } 
            client.get_local_storage().del(_key);  
            log::Info()<<"delete ok,need to commit " << _key;
        }
    private:
        std::string _key; 
    };
    typedef DeleteMethod2 DeleteMethod;
}


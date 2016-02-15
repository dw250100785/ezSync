/**
* $Id: cloud_version_history2.cpp 866 2014-05-07 10:45:07Z caoyangmin $ 
* @author caoyangmin(caoyangmin@gmail.com)
* @brief
*/
#include <ctype.h>
#include <errno.h>
#include "boost/filesystem.hpp"
#include "boost/property_tree/ptree.hpp"
#include "boost/property_tree/xml_parser.hpp"
#include "boost/algorithm/string.hpp"
#include "boost/lexical_cast.hpp"
#include "ezsync/cloud_version_history2.h"
#include "ezsync/verify_true.h"
#include "ezsync/log.h"
#include "ezsync/TransferInterface.h"
#include "ezsync/verify_transfer_result.h"
//TODO: history等文件缺失容错
using namespace ezsync;
//TODO: 小文件太多
CloudVersionHistory2::CloudVersionHistory2(std::shared_ptr<TransferInterface> transfer,const std::string&local_root,const std::string&remote_root)
    :_local_root(local_root)
    ,_remote_root(remote_root)
    ,_transfer(transfer)
    ,_current_cursor("0-"){
        reload();
}
void CloudVersionHistory2::sign_commit(const CommitItems & changed){
    time_t now = time(0);
    _sign_commit.put(filter_key("ezsync/mtime"),now);
    for(std::map<std::string,Entry>::const_iterator it = changed.begin(); it != changed.end(); it++){
        boost::optional<boost::property_tree::ptree& > find =
            _sign_commit.get_child_optional(filter_key("ezsync/"+it->first));
        if(find && find->get<std::string>(filter_key("md5")) == it->second.md5){
            continue;//未修改
        }
        boost::property_tree::ptree enrty;
        enrty.put(filter_key("mtime"),now);
        enrty.put(filter_key("md5"),it->second.md5);
        _sign_commit.put_child(filter_key("ezsync/"+it->first),enrty);
    }
    boost::filesystem::create_directories(get_local_path(".ezsync/version/"));
    boost::property_tree::write_xml(get_local_path(".ezsync/version/sign_commit"),_sign_commit); 
}
// 更新版本时,先把版本信息保存到一个新的文件,然后修改当前版本的指向
// 此方法可以防止异常终止导致版本库损坏
unsigned int CloudVersionHistory2::commit(const CommitItems & changed){
    VERIFY_TRUE(!changed.empty());
    refresh();
    unsigned int to_version = std::get<0>(split_version_str(_remote_version))+1;
    sign_commit(changed);

    time_t now = time(0);

    //控制每次上传的xml大小,将过大的xml分成多个文件上传
    std::map<std::string,Entry>::const_iterator it = changed.begin();
    size_t xml_size = 0; //估算的xml大小
    std::vector<std::pair<std::string, std::string> > to_upload;
    std::string new_version_num ;
    std::string new_version_str;
    std::string from = _remote_version;

    boost::property_tree::ptree doc;
    doc.put(filter_key("ezsync/mtime"),now);
    std::list<std::string> temp_versions;

    boost::filesystem::create_directories(get_local_path(".ezsync/version/temp2"));
    boost::filesystem::create_directories(get_local_path(".ezsync/version/history"));
    while(it != changed.end()){
        boost::property_tree::ptree enrty;
        boost::optional<boost::property_tree::ptree& > sign =
            _sign_commit.get_child_optional(filter_key("ezsync/"+it->first));
        enrty.put(filter_key("mtime"),sign?sign->get<unsigned int>(filter_key("mtime")):now);
        //enrty.put(filter_key("storage_path"),it->second.storage_path);
        enrty.put(filter_key("md5"),it->second.md5);
        //enrty.put(filter_key("content"),it->second.content);
        enrty.put(filter_key("size"),it->second.size);
        enrty.put(filter_key("leaf"),1);
        enrty.put(filter_key("version"),to_version);
        doc.put_child(filter_key("ezsync/"+it->first),enrty);
        xml_size += (it->first.size()*2 +10 );
        xml_size += 256;
        it++;
        if(it == changed.end() || xml_size > 32764){
            doc.put(filter_key("ezsync/from"),from);
            doc.put(filter_key("ezsync/version"),to_version );

            std::ostringstream xml;
            boost::property_tree::write_xml(xml,doc);
            new_version_num = boost::lexical_cast<std::string>(to_version);
            new_version_str = new_version_num+"-"+get_text_md5(xml.str());
            to_upload.push_back(std::make_pair(xml.str(),
                get_remote_path("/$ezsync/version/history/"+new_version_str)));
            log::Debug()<< "segment xml " << xml.str().size() << "," << xml_size;
            //更新本地版本号
            //1.创建版本文件,以x-md5 的形式命名
            //2.修改当前版本号
            boost::filesystem::path version_path(get_local_path(".ezsync/version/temp2/")+new_version_str);
            temp_versions.push_back(new_version_str);
            std::ofstream fstream(version_path.string().c_str(), std::ios::trunc|std::ios::binary);
            fstream.write(xml.str().c_str(),xml.str().size());
            fstream.close();
            if(it != changed.end()){
                from = new_version_str;
                to_version ++;
                xml_size=0;
                doc.clear();
            }
        }
    }
  
    //更新云端版本号
    //1.上传版本文件, 以x-md5 的形式命名
    //2.修改当前版本号, 以x-md5 的形式命名
    //3.删除无意义的版本,如果一个版本中包含的修改项全部被后续的版本覆盖,则这个版本可以删除
    error_info_t e;
    std::vector<return_info_t> return_list;
    VERIFY_TRANSFER_RESULT(_transfer->batch_upload_from_buffer( to_upload,false,return_list,e)|| e.error_code == FILE_EXIST,e);
    size_t pos = 0;
    std::for_each(return_list.begin(),return_list.end(),[&pos,&return_list,&to_upload](const return_info_t& it){
        VERIFY_TRUE(it.is_success) 
            << to_upload[pos].second
            << it.error_info.error_code 
            <<" "<<it.error_info.error_msg;
        pos++;
    });
    if(_remote_version == "0-"){ //TODO: 没有保证原子性
        VERIFY_TRANSFER_RESULT(_transfer->upload_from_buffer( "0" //TODO:bug:无法上传空buffer
            ,get_remote_path("/$ezsync/version/current/"+new_version_str)
            ,false,e) || e.error_code == FILE_EXIST,e);
        log::Debug()<<"upload remote version "<<new_version_str;
    }else{
        VERIFY_TRANSFER_RESULT(_transfer->move(get_remote_path("/$ezsync/version/current/"+_remote_version)
            ,get_remote_path("/$ezsync/version/current/"+new_version_str),e),e);
        log::Debug()<<"upload remote version "<<_remote_version<<"->"<<new_version_str;
    }
    
    std::for_each(temp_versions.begin(),temp_versions.end(),[this](const std::string& it){
        boost::filesystem::rename(get_local_path(".ezsync/version/temp2/"+it),
        get_local_path(".ezsync/version/history/"+it));
        valid_version(it);
    });
    _remote_version = new_version_str;
     while(!_nil_versions.empty()){//TODO: 批量删除
        boost::filesystem::remove(get_local_path(".ezsync/version/history/"+_nil_versions.front()));
        if(_transfer->del(get_remote_path("/$ezsync/version/history/"+_nil_versions.front()),e)){
            log::Info()<<"drop nil version "<<_nil_versions.front() <<" ok";
        }else if(e.error_code == FILE_NOT_EXIST){
            log::Debug()<<"drop nil version "<<_nil_versions.front() <<" FILE_NOT_EXIST";
        }else{
            log::Warn()<<"drop nil version "<<_nil_versions.front()<<" failed with "<<e.error_code <<" "<<e.error_msg;
            break;
        }
        _nil_versions.pop_front();
    }

    return to_version;
}
//TODO: 远端缺少一个版本时,本地也需删除
VersionEntry CloudVersionHistory2::get_entry(const std::string& key)const{
    boost::optional<const boost::property_tree::ptree& > entry =_doc.get_child_optional(filter_key("ezsync/"+key));
    if(entry == NULL) return VersionEntry();

    VersionEntry res;
    res.modified_time = entry->get<unsigned int>(filter_key("mtime"));
    //res.storage_path = entry->get<std::string>(filter_key("storage_path"));
    res.md5 = entry->get<std::string>(filter_key("md5"));
    //res.content = entry->get<std::string>(filter_key("content"));
    res.size = entry->get<unsigned int>(filter_key("size"));
    res.version = entry->get<unsigned int>(filter_key("version"));
    return res;
}
VersionEntry CloudVersionHistory2::get_latest_entry(const std::string& key)const{
    boost::optional<const boost::property_tree::ptree& > entry =_latest.get_child_optional(filter_key("ezsync/"+key));
    if(entry == NULL) return get_entry(key);
    VersionEntry res;
    res.modified_time = entry->get<unsigned int>(filter_key("mtime"));
    //res.storage_path = entry->get<std::string>(filter_key("storage_path"));
    res.md5 = entry->get<std::string>(filter_key("md5"));
    //res.content = entry->get<std::string>(filter_key("content"));
   res.size = entry->get<unsigned int>(filter_key("size"));
    res.version = entry->get<unsigned int>(filter_key("version"));
    return res;
}
void CloudVersionHistory2::get_map_from_tree(std::map<std::string, VersionEntry>& map, 
    const boost::property_tree::ptree& doc,const std::string&recursion/*=""*/)const{
        if(is_leaf(doc)){

            VersionEntry entry;
            entry.modified_time = doc.get<unsigned int>(filter_key("mtime"));
            entry.md5 = doc.get<std::string>(filter_key("md5"));
            //entry.content = it->second.get<std::string>(filter_key("content"));
            entry.size = doc.get<unsigned int>(filter_key("size"));
            entry.version = doc.get<unsigned int>(filter_key("version"));
            map[recursion] = entry;

            return ;
        }
        for(boost::property_tree::ptree::const_iterator it = doc.begin(); it != doc.end(); it++){
            if(is_leaf(it->second)){
                VersionEntry entry;
                entry.modified_time = it->second.get<unsigned int>(filter_key("mtime"));
                entry.md5 = it->second.get<std::string>(filter_key("md5"));
                entry.size = it->second.get<unsigned int>(filter_key("size"));
                entry.version = it->second.get<unsigned int>(filter_key("version"));
                map[decode_key(recursion.empty()?it->first:recursion+"/"+it->first)] = entry;

            }else{
                get_map_from_tree(map,it->second,recursion.empty()?it->first:(recursion+"/"+it->first));
            }
        }
}
std::string CloudVersionHistory2::get_from_version(const std::string& version)const{
    boost::property_tree::ptree doc;
    boost::property_tree::read_xml(get_local_path(".ezsync/version/history/")
        +boost::lexical_cast<std::string>(version),doc);
    return doc.get<std::string>(filter_key("ezsync/from"));
}
std::string CloudVersionHistory2::get_from_version(const boost::property_tree::ptree& doc)const{
    return doc.get<std::string>(filter_key("ezsync/from"));
}
//boost::filesystem::path CloudVersionHistory2::get_current_version_file()const{
//    boost::filesystem::path root(get_local_path(".ezsync/version/current"));
//    if(!boost::filesystem::exists(root)){
//        return root/"0-";
//    }
//    boost::filesystem::path version_file = root/"0-";
//    unsigned int max_version = 0;
//    for(boost::filesystem::directory_iterator it(root); it != boost::filesystem::directory_iterator(); it++){
//        /*if(boost::filesystem::is_directory(*it))*/{
//            std::vector<std::string> data;   
//            std::string filename = it->filename();
//            boost::algorithm::split( data, filename, boost::is_any_of("-") );// 5-f12a3d4c5e6a7c8af12a3d4c5e6a7c8a
//            if(data.size()>=2){
//                unsigned int temp = (unsigned int)strtol(data[0].c_str(),0,10);
//                if(temp >= max_version){
//                    max_version = temp;
//                    version_file = *it;
//                }
//            }
//        }
//    }
//    return version_file;
//}
void CloudVersionHistory2::merge_to(const boost::property_tree::ptree& child, 
    boost::property_tree::ptree& merged,
    const PreOverwrite& pre_overwrite/*=PreOverwrite() */,
    const std::string &recursion /*= ""*/) const{
    //unsigned int ver = child.get<unsigned int>(filter_key("ezsync/version"));
    //if( ver > merged.get<unsigned int>(filter_key("ezsync/version"))){
    //        merged.put(filter_key("ezsync/version"),ver);
    //}
    for(boost::property_tree::ptree::const_iterator it = child.begin(); it != child.end(); it++){
        if(is_leaf(it->second)){
            boost::optional<boost::property_tree::ptree&> entry = merged.get_child_optional(it->first);
            if(entry == NULL || it->second.get<unsigned int>("version") >= entry->get<unsigned int>("version")){//大版本可以替换小版本
                //if(entry && it->second.get<unsigned int>("version") == entry->get<unsigned int>("version")){
                    //log::Warn()<<"different version has the same version num " << decode_key(recursion.empty()?it->first:(recursion+"/"+it->first));
                //}
                if(entry && pre_overwrite){
                    pre_overwrite(entry->get<unsigned int>("version"),decode_key(recursion.empty()?it->first:(recursion+"/"+it->first)));
                }
                merged.put_child(it->first,it->second);
            }
        }else if(it->second.empty()){
            if(merged.get_optional<std::string>(it->first) == NULL){
                merged.put(it->first,it->second.data());
            }
        }else{
            if(merged.get_child_optional(it->first) == NULL){
                merge_to(it->second, merged.put_child(it->first,boost::property_tree::ptree()),pre_overwrite,recursion.empty()?it->first:(recursion+"/"+it->first));
            }else{
                merge_to(it->second, merged.get_child(it->first),pre_overwrite,recursion.empty()?it->first:(recursion+"/"+it->first));
            }
        }
    }
}
bool CloudVersionHistory2::is_leaf(const boost::property_tree::ptree& child)const{
    boost::optional< const boost::property_tree::ptree& > entry = 
        child.get_child_optional(filter_key("leaf"));
    if(!entry)return false;
    return entry->data() == "1";
}

//预处理文件名,便于作为xml\json等格式的key
boost::property_tree::ptree::path_type CloudVersionHistory2::filter_key(const std::string& key)const {
    std::vector<std::string> data;   
    boost::algorithm::split( data,key, boost::algorithm::is_any_of("\\/") );
    if(data.size() == 0) return boost::property_tree::ptree::path_type();
    std::string dest;
    for(std::vector<std::string>::iterator it = data.begin(); it!=data.end(); it++){
        if(it != data.begin()){
            dest += "/";
        }
        dest += encode_key(*it);
    }
    return boost::property_tree::ptree::path_type(dest,'/');// TODO
}
std::string CloudVersionHistory2::get_local_path(const std::string&path)const{
    if(path.empty()) return _local_root;
    if(path[0] != '/')
        return _local_root.empty()?path:_local_root+"/"+path;
    else
        return _local_root.empty()?path:_local_root+path;
}
std::string CloudVersionHistory2::get_remote_path(const std::string&path)const{
    if(path.empty()) return _remote_root;
    if(path[0] != '/')
        return _remote_root.empty()?path:_remote_root+"/"+path;
    else
        return _remote_root.empty()?path:_remote_root+path;
}

void CloudVersionHistory2::add_delete_mark(const std::string& key){
    VersionEntry ori = get_entry(key);
    if(ori.is_exist()){ //如果已经在版本中,需要标记为删除
        boost::filesystem::create_directories(get_local_path(".ezsync/deleted"));
        std::ostringstream oss;
        oss << key << "$" << ori.version<<"-"<< ori.md5;
        FILE* created = fopen(get_local_path(std::string(".ezsync/deleted/")+encode_key(oss.str())).c_str() ,"w");
        VERIFY_TRUE(created != 0)<< "add_delete_mark " << key << "failed with " << errno;
        fclose(created);
    }
}
bool CloudVersionHistory2::has_delete_mark(const std::string& key,const VersionEntry&ori){
    std::ostringstream oss;
    oss << key << "$" << ori.version<<"-"<< ori.md5;
    return boost::filesystem::exists( get_local_path(std::string(".ezsync/deleted/")+encode_key(oss.str())));
}
void CloudVersionHistory2::clear_delete_marks(){
    boost::filesystem::remove_all(get_local_path(".ezsync/deleted/"));
}
std::map<std::string, VersionEntry> CloudVersionHistory2::get_delete_marks(){
    std::map<std::string, VersionEntry> marks;
    std::map<std::string,Entry> changed;
    boost::filesystem::path path(get_local_path(".ezsync/deleted"));
    if(!boost::filesystem::exists(path)) return std::map<std::string,VersionEntry>();
    for(boost::filesystem::directory_iterator it(path); 
        it != boost::filesystem::directory_iterator(); 
        it++){
            if(!boost::filesystem::is_directory(*it)){
                std::string filename = decode_key(it->path().filename().string());
                std::size_t post = filename.rfind("$");
                VERIFY_TRUE(post != std::string::npos) << "invalid file name :without $";
                std::vector<std::string> data;
                std::string info = filename.substr(post+1);
                filename = filename.substr(0,post);
                boost::algorithm::split( data,info, boost::algorithm::is_any_of("-") );
                VERIFY_TRUE(data.size() >=2 ) << "invalid file name :without $";
                VersionEntry ori = get_entry(filename);
                if(ori.version == strtol(data[0].c_str(),0,10)){ //TODO: 还需要判断MD5?
                    marks.insert(std::make_pair(filename, ori)); 
                }
            }
    }
    return marks;
}
//修改时间相关
void CloudVersionHistory2::set_custom_value(const std::string& key,const std::string& value)
{
    boost::filesystem::create_directories(get_local_path(".ezsync/value"));
    std::ofstream f(get_local_path(".ezsync/value/"+key),std::ios::trunc|std::ios::binary);
    f.write(value.c_str(),value.size());
}
std::string CloudVersionHistory2::get_custom_value(const std::string& key)
{
    std::ifstream f(get_local_path(".ezsync/value/"+key),std::ios::binary);
    if(f.is_open()) {
        std::string v;
        f >> v;
        return v;
    }else{
        return "";
    }
}
void CloudVersionHistory2::refresh(){
    std::list<enum_item_t> items;
    unsigned int to_version = 1;
    error_info_t e;
    VERIFY_TRANSFER_RESULT(_transfer->list(get_remote_path("/$ezsync/version/current"),items,e) || e.error_code == FILE_NOT_EXIST,e);
    unsigned int max_version = 0;
    std::string version_name="0-";
    for(std::list<enum_item_t>::iterator it = items.begin(); it != items.end(); it++){
        log::Debug()<<"list remote version :"<<it->path;
        std::tuple<unsigned int,std::string> version = split_version_str(it->path);
        if(std::get<0>(version) >= max_version){
            max_version = std::get<0>(version);
            version_name = it->path;
        }
    }
    //如果远端版本比上次低，可能上传的版本还没有同步到分布式数据库的其他节点上
    if(std::get<0>(split_version_str(version_name)) <  std::get<0>(split_version_str(_remote_version)) ){
        log::Warn() << "remote version " << version_name << " < local version " << _remote_version;
        version_name = _remote_version;
    }
    if(_remote_version != version_name){
        _remote_version = version_name;
        _current_cursor = _remote_version;
    }
}
void CloudVersionHistory2::reload(){
    if(boost::filesystem::exists(get_local_path(".ezsync/version/history"))){
        //加载所有版本文件
        unsigned int max_v = 0;
        for(boost::filesystem::directory_iterator it(get_local_path(".ezsync/version/history")); 
            it != boost::filesystem::directory_iterator(); 
            it++){
                if(!boost::filesystem::is_directory(*it)){
                    valid_version(it->path().filename().string());
                    unsigned int  v = std::get<0>(split_version_str(it->path().filename().string()));
                    if(v >max_v){
                        max_v = v;
                        _current_cursor = it->path().filename().string();
                    }
                }
        }
    }
    if(boost::filesystem::exists(get_local_path(".ezsync/version/temp"))){
        for(boost::filesystem::directory_iterator it(get_local_path(".ezsync/version/temp")); 
            it != boost::filesystem::directory_iterator(); 
            it++){
                if(!boost::filesystem::is_directory(*it)){
                    std::shared_ptr<boost::property_tree::ptree> next(new boost::property_tree::ptree());
                    try{
                        boost::property_tree::read_xml(it->path().string(),*next);
                    }catch(const boost::property_tree::xml_parser_error &e){
                        log::Error()<<"file corrupted "<<e.what();
                        boost::filesystem::remove(get_local_path(it->path().string()));
                    }
                    _temp_cache[it->path().filename().string()] = next;
                    merge_to(*next,_latest);
                }
        }
    }

    if(boost::filesystem::exists(get_local_path(".ezsync/version/sign_commit"))){
        try{
            boost::property_tree::read_xml(get_local_path(".ezsync/version/sign_commit"),_sign_commit); 
        }catch(const std::exception& e){
            log::Warn()<<"read sign_commit failed " << e.what();
        }
    }
}
void CloudVersionHistory2::update(CloudVersionHistory2::UpdateRout rout){
    std::list<std::string> pending_version; //正在处理的版本
    std::map<std::string, VersionEntry> total_updated;
    std::string current_cursor = _current_cursor;
    std::list<std::string> completed_item;  //已经处理的条目
    while(current_cursor != "0-"){
        if(is_updated(current_cursor)){
            std::string temp = get_from_version(*_temp_cache[current_cursor]);
            VERIFY_TRUE(current_cursor != temp) << "endless loop";
            current_cursor = temp;
            continue;
        }
        //获取未下载的版本
        std::string path = get_local_path(".ezsync/version/history/"+current_cursor);
        if(!boost::filesystem::exists(path)){
            //下载
            std::shared_ptr<boost::property_tree::ptree> doc = _temp_cache[current_cursor];
            std::map<std::string, VersionEntry> changed;
            if(!doc || !boost::filesystem::exists(get_local_path(".ezsync/version/temp/"+current_cursor))){
                if(!download(current_cursor,current_cursor))continue;
                try{
                    doc.reset(new boost::property_tree::ptree());
                    boost::property_tree::read_xml(get_local_path(".ezsync/version/temp/"+current_cursor),*doc);
                    _temp_cache[current_cursor] = doc;
                }catch(const boost::property_tree::xml_parser_error &e){
                    log::Error()<<"file corrupted "<<e.what();
                    boost::filesystem::remove(path);
                    break;
                }
                merge_to(*doc,_latest);
            }
            boost::optional<boost::property_tree::ptree& > child = doc->get_child_optional(filter_key("ezsync"));
            if(child)get_map_from_tree(changed,*child);

            std::map<std::string, VersionEntry> valid_changed;
            for(std::map<std::string, VersionEntry>::iterator it = changed.begin();
                it != changed.end();it++){
                    VersionEntry ori =  get_entry(it->first);
                    if((!ori.is_exist() || it->second.version > ori.version ) && total_updated.find(it->first) == total_updated.end()){
                        valid_changed[it->first] = it->second;
                        total_updated[it->first] = it->second;
                    }
            }
            bool to_continue = true;
            if(!valid_changed.empty())
                to_continue = rout(valid_changed,completed_item);
            if(!to_continue){
                log::Debug()<< "update canceled @ version " << current_cursor;
                break;
            }else{
                pending_version.push_back(current_cursor);
            }
            VERIFY_TRUE(current_cursor != get_from_version(*doc)) << "endless loop";
            current_cursor = get_from_version(*doc);
        }else{
            VERIFY_TRUE(false)<<"OMG!";
        }
    }
    if(pending_version.empty()) return;
    //应用全部更新,一般在此次回调中真正开始下载
    rout(std::map<std::string, VersionEntry>(),completed_item);
    //找出未更新完成的版本,剔除后剩下的是已更新完成的版本
    //1.找出未更新的项目
    std::for_each(completed_item.begin(),completed_item.end(),[&total_updated](const std::string& it){
        total_updated.erase(it);
    });
    //2.剔除未更新完成的版本
    std::for_each(total_updated.begin(),total_updated.end(),[this,&pending_version](std::map<std::string, VersionEntry>::reference it){
        std::string find ;
        for(std::list<std::string>::iterator j = pending_version.begin();
            j != pending_version.end();){
                std::list<std::string>::iterator temp = j++;
                if(std::get<0>(this->split_version_str(*temp)) == it.second.version){
                    pending_version.erase(temp);
                }
        }
    });
    boost::filesystem::create_directories(get_local_path(".ezsync/version/history/"));
    for(std::list<std::string>::iterator it=pending_version.begin();
        it != pending_version.end();it++){
            boost::filesystem::rename(get_local_path(".ezsync/version/temp/"+*it)
                , get_local_path(".ezsync/version/history/"+*it));
            valid_version(*it);
            _current_cursor = get_from_version(*it);
    }
}
void CloudVersionHistory2::valid_version(const std::string& version){
    VERIFY_TRUE(!is_updated(version));
    unsigned int  nver = std::get<0>(split_version_str(version));
    _updated_version[nver]=version;
    log::Debug()<<"valid_version "<<version;
    std::shared_ptr< boost::property_tree::ptree > next (new boost::property_tree::ptree());
    try{
        boost::property_tree::read_xml(get_local_path(".ezsync/version/history/"+version),*next);
    }catch(const boost::property_tree::xml_parser_error &e){
        log::Error()<<"file corrupted "<<e.what();
        boost::filesystem::remove(get_local_path(".ezsync/version/history/"+version));
        throw e;
    }
    _temp_cache[version] = next;
    merge_to(*next,_latest);
    _version_detail[nver].first = version;
    get_map_from_tree(_version_detail[nver].second,*next);

    merge_to(*next, _doc,[this](unsigned int ver,const std::string&key){
        //监听覆盖项目,如果一个版本的所有项目均被覆盖,则这个版本可以删除
        CloudVersionHistory2::VersionDetail::iterator it = _version_detail.find(ver);
        if(it != _version_detail.end()){
            it->second.second.erase(key);
            if(it->second.second.empty()){
                log::Debug()<<"lose efficacy version " << it->second.first;
                _nil_versions.push_back(it->second.first);
                _version_detail.erase(it);
            }
        }
    });

}
void CloudVersionHistory2::refresh_history_version_list(){
    std::list<enum_item_t> items;
    error_info_t e;
    VERIFY_TRANSFER_RESULT(_transfer->list(get_remote_path("/$ezsync/version/history"),items,e) || e.error_code == FILE_NOT_EXIST,e);
    unsigned int max_v= 0;
    unsigned int min_v= 0;
    log::Debug()<<"list version history, total count:" << items.size();
    std::for_each(items.begin(),items.end(),[this,&max_v,&min_v](const enum_item_t & it){
        unsigned int  v = std::get<0>(split_version_str(it.path));
        max_v = std::max(max_v,v);
        min_v = std::min(min_v,v);
        _history_versions[v] = it.path;
    });
    //同时记录不存在的版本
    for(unsigned int i = min_v+1 ; i < max_v;i++){
        _history_versions.insert(std::make_pair(i,""));
    }
}
bool CloudVersionHistory2::download(const std::string& version,std::string& before_version){
    std::string path = get_local_path(".ezsync/version/temp/"+version);
    if(boost::filesystem::exists(path) ){
        return true;
    }
    //版本可能不存在
    std::vector<std::pair<std::string, std::string> > to_download;
    std::vector<unsigned int> to_download_version;
    unsigned int nver = std::get<0>(split_version_str(version));
    VERIFY_TRUE(nver != 0);
    std::map<unsigned int,std::string>::iterator it = _history_versions.find(nver);
    if(it != _history_versions.end() && it->second.empty()){//版本不存在
        //确定前一个最近的版本
        std::map<unsigned int,std::string>::iterator i;
        unsigned int loop = nver;
        before_version = "0-";
        while((i=_history_versions.find(loop--)) != _history_versions.end()){
            if(!i->second.empty()) {
                before_version = i->second;
                break;
            }
        }
        
        //更新_history_versions
        return false;
    }


    //每次下载比所需的更多的版本文件
    //预测接下来可能要用到的版本,并一次性下载
    //版本号可能是不连续的
    //_history_versions中记录了版本号和版本文件的对应,如果不在_history_versions中,则考虑先更新_history_versions
    //TODO: _history_versions记录在文件中
    if(_history_versions.find(nver) == _history_versions.end()){
        //版本不在列表中
        if(nver != 0 /*&& _history_versions.find(nver-1) == _history_versions.end()*/){//前一个版本也不在列表中,
            //更新_history_versions
            refresh_history_version_list();
        }
    }
    VERIFY_TRUE((it = _history_versions.find(nver)) != _history_versions.end());
    if( it->second.empty()){
        return download(version,before_version);
    }
    to_download_version.push_back(nver);
    to_download.push_back(std::make_pair(get_remote_path("/$ezsync/version/history/"+ version),
                    get_local_path(".ezsync/version/temp/"+version)));
    for(unsigned int i = nver-1;
        i != 0 && to_download.size() < s_max_download_count;
        i--){
            std::map<unsigned int,std::string>::iterator it = _history_versions.find(i);
            if( it != _history_versions.end() 
                && !it->second.empty() 
                && !is_updated(i) 
                && !boost::filesystem::exists(get_local_path(".ezsync/version/temp/"+it->second))){
                to_download.push_back(std::make_pair(get_remote_path("/$ezsync/version/history/"+it->second),
                    get_local_path(".ezsync/version/temp/"+it->second)));
                to_download_version.push_back(i);
            }
    }
    boost::filesystem::create_directories(get_local_path(".ezsync/version/temp/"));
    error_info_t e;
    std::vector<return_info_t> return_list;
    _transfer->batch_download_to_file(to_download,return_list,e);
    VERIFY_TRUE(return_list.size() == to_download.size()) <<"batch_download_to_file return size != request size";
    for(size_t i = 0;i<return_list.size();i++){
        if(return_list[i].error_info.error_code == FILE_NOT_EXIST){
            _history_versions[to_download_version[i]] = "";//记录不存在的情况
        }
        if(!return_list[i].is_success){
            log::Debug()<< to_download[i].first
                << " batch download version failed with " 
                << return_list[i].error_info.error_code 
                <<" "
                << return_list[i].error_info.error_msg
                << " "
                << e.error_msg;
        }else{
            log::Debug()<< to_download[i].first
                << " batch download version ok "; 
        }
    }
    if(return_list[0].error_info.error_code == FILE_NOT_EXIST){
        return download(version,before_version);
    }
    VERIFY_TRANSFER_RESULT(return_list[0].is_success,return_list[0].error_info);
    return true;
}
bool CloudVersionHistory2::is_updated(unsigned int version)const{
    return _updated_version.find(version) != _updated_version.end();
}
bool CloudVersionHistory2::is_updated(const std::string& version)const{

    std::map<unsigned int, std::string>::const_iterator it = _updated_version.find( std::get<0>(split_version_str(version)));
    if(it == _updated_version.end()){
        return false;
    }
    return it->second == version;
}
std::tuple<unsigned int,std::string> CloudVersionHistory2::split_version_str(const std::string& version){
    std::vector<std::string> data;   
    boost::algorithm::split( data, version, boost::is_any_of("-") );// 5-f12a3d4c5e6a7c8af12a3d4c5e6a7c8a
    if(data.size() == 0){
        return std::make_tuple(0,"");
    }
    return std::make_tuple(strtol(data[0].c_str(),0,10),(data.size()>=2)?data[1]:"");
}
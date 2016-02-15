/**
* $Id$ 
* @author caoyangmin(caoyangmin@gmail.com)
* @brief
*/
#include <ctype.h>
#include <errno.h>
#include "boost/filesystem.hpp"
#include "boost/property_tree/ptree.hpp"
#include "boost/property_tree/xml_parser.hpp"
#include "boost/algorithm/string.hpp"
#include "ezsync/file_version_history.h"
#include "ezsync/verify_true.h"
#include "ezsync/log.h"
//TODO: history等文件缺失容错
using namespace ezsync;
FileVersionHistory::FileVersionHistory(const std::string&root)
    :_root(root){
        reload();
}
unsigned int FileVersionHistory::get_version_num()const{
    return _doc.get<unsigned int>(filter_key("ezsync/version"));
}
unsigned int FileVersionHistory::get_version_num(const std::string& key){
    boost::optional< unsigned int> r = _doc.get_optional<unsigned int>(filter_key("ezsync/"+key+"/version"));
    while(r==NULL && append_next_history(_doc)){ //当前版本中未找到指定文件,还需要到历史版本中查找
        r = _doc.get_optional<unsigned int>(filter_key("ezsync/"+key+"/version"));
    }
    VERIFY_TRUE(!!r)<<key<<" not exits";
    return *r;
}
unsigned int FileVersionHistory::get_modified_time()const{
    return _doc.get<unsigned int>(filter_key("ezsync/mtime"));
}
// 更新版本时,先把版本信息保存到一个新的文件,然后修改当前版本的指向
// 此方法可以防止异常终止导致版本库损坏
void FileVersionHistory::upgrade(unsigned int to_version,const std::map<std::string,Entry>& changed){
    if(changed.empty() && to_version == get_version_num()) return;
   
    boost::filesystem::path max_version = _current_version;
    time_t now = time(0);
    boost::property_tree::ptree doc;
    doc.put(filter_key("ezsync/mtime"),now);
    if( get_version_num() > to_version){
        //如果远端版本比本地版本小，说明远端数据被人为清空过，本地需要重置版本信息
        //TODO:比较版本号的方法并不能反映全部“远端数据清空”的问题，比如清空后可能有
        //其他客户端重新进行同步，版本号最终会递增上来。以后考虑用版本号+md5的方式识别
        doc.put(filter_key("ezsync/from"),"0-");
    }else{
        doc.put(filter_key("ezsync/from"),max_version.filename() );
    }
    
    doc.put(filter_key("ezsync/version"),to_version );
    for(std::map<std::string,Entry>::const_iterator it = changed.begin(); it != changed.end(); it++){
        boost::property_tree::ptree enrty;
        enrty.put(filter_key("mtime"),now);
        //enrty.put(filter_key("storage_path"),it->second.storage_path);
        enrty.put(filter_key("md5"),it->second.md5);
        //enrty.put(filter_key("content"),it->second.content);
        enrty.put(filter_key("size"),it->second.size);
        enrty.put(filter_key("leaf"),1);
        enrty.put(filter_key("version"),to_version);
        doc.put_child(filter_key("ezsync/"+it->first),enrty);
    }
    std::ostringstream xml;
    boost::property_tree::write_xml(xml,doc);
    std::ostringstream new_version;
    new_version << to_version << "-" << get_text_md5(xml.str());
    boost::filesystem::path version_path(get_absolute_path(".ezsync/version/history/")+new_version.str());
    boost::filesystem::create_directories(version_path.parent_path());
    std::ofstream fstream(version_path.string().c_str(), std::ios::trunc|std::ios::binary);
    fstream.write(xml.str().c_str(),xml.str().size());
    fstream.close();
    //"0-"第一次更新,需要创建文件
    if(max_version.filename()=="0-"){//! TODO: 这里不能保证原子性
        boost::filesystem::create_directories(max_version.parent_path());
        FILE* created = fopen(max_version.string().c_str() ,"w");
        VERIFY_TRUE(created != 0)<< "create " << max_version.string() << " failed with " << errno;
        fclose(created);
    }
    VERIFY_TRUE(0==rename(max_version.string().c_str(),(max_version.parent_path()/new_version.str()).string().c_str()))
        << "rename failed with " <<errno;
    reload();
}
VersionEntry FileVersionHistory::get_entry(const std::string& key){
    boost::optional<boost::property_tree::ptree& > entry =_doc.get_child_optional(filter_key("ezsync/"+key));
    while(entry==NULL && append_next_history(_doc)){ //当前版本中未找到指定文件,还需要到历史版本中查找
        entry =_doc.get_child_optional(filter_key("ezsync/"+key));
    }
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
std::map<std::string, VersionEntry> FileVersionHistory::get_all_entries(const std::string& path){
    return get_changed_from(0,path);
}
std::map<std::string, VersionEntry> FileVersionHistory::get_changed_from(unsigned int version, const std::string& path){
    std::map<std::string, VersionEntry> changed;
    std::string from = _current_version.filename().string();
    while(get_from_version(_doc)>version && append_next_history(_doc)) {
    }
    boost::optional<boost::property_tree::ptree& > child = _doc.get_child_optional(filter_key("ezsync/"+path));
    if(child){
        get_map_from_tree(changed,*child,version,path);
    }
    return changed;
}
void FileVersionHistory::discard_history(){
    if(_doc.get<std::string>(filter_key("ezsync/from"))=="0-") return;
    boost::filesystem::path max_version = _current_version;
    std::string from = max_version.filename().string();
    while(append_next_history(_doc)) {
    }
    std::ostringstream xml;
    boost::property_tree::write_xml(xml,_doc);

    std::string new_version = _doc.get<std::string>(filter_key("ezsync/version"))+"-"+get_text_md5(xml.str());
    std::ofstream fstream((get_absolute_path(".ezsync/version/history/")+new_version).c_str(), std::ios::trunc|std::ios::binary);
    fstream.write(xml.str().c_str(),xml.str().size());
    fstream.close();
    VERIFY_TRUE(0==rename(max_version.string().c_str(),(max_version.parent_path()/new_version).string().c_str()))
        << "rename failed with" <<errno;
    //删除其他历史版本
    boost::filesystem::path root(get_absolute_path(".ezsync/version/history"));
    VERIFY_TRUE(boost::filesystem::exists(root))<<" version/history not exists";
    for(boost::filesystem::directory_iterator it(root); it != boost::filesystem::directory_iterator(); it++){
        if(!boost::filesystem::is_directory(*it)){
            if(it->path().filename().string()!=new_version){
                remove(it->path().string().c_str());
                log::Debug()<<"discard_history "<<it->path().string();
            }
        }
    }
    reload();
}

void FileVersionHistory::get_map_from_tree(std::map<std::string, VersionEntry>& map, 
    const boost::property_tree::ptree& doc,
    unsigned int from_version,const std::string&recursion/*=""*/)const{

        if(is_leaf(doc)){
            if(doc.get<unsigned int>(filter_key("version")) > from_version){
                VersionEntry entry;
                entry.modified_time = doc.get<unsigned int>(filter_key("mtime"));
                entry.md5 = doc.get<std::string>(filter_key("md5"));
                //entry.content = it->second.get<std::string>(filter_key("content"));
                entry.size = doc.get<unsigned int>(filter_key("size"));
                entry.version = doc.get<unsigned int>(filter_key("version"));
                map[recursion] = entry;
            }
            return ;
        }
        for(boost::property_tree::ptree::const_iterator it = doc.begin(); it != doc.end(); it++){
            if(is_leaf(it->second)){
                if(it->second.get<unsigned int>(filter_key("version")) > from_version){
                    VersionEntry entry;
                    entry.modified_time = it->second.get<unsigned int>(filter_key("mtime"));
                    entry.md5 = it->second.get<std::string>(filter_key("md5"));
                    //entry.content = it->second.get<std::string>(filter_key("content"));
                    entry.size = it->second.get<unsigned int>(filter_key("size"));
                    entry.version = it->second.get<unsigned int>(filter_key("version"));
                    map[decode_key(recursion.empty()?it->first:recursion+"/"+it->first)] = entry;
                }
            }else{
                get_map_from_tree(map,it->second,from_version,recursion.empty()?it->first:(recursion+"/"+it->first));
            }
        }
}
unsigned int FileVersionHistory::get_from_version(const boost::property_tree::ptree& doc)const{
    std::string from = doc.get<std::string>(filter_key("ezsync/from"));
    std::vector<std::string> data;   
    boost::algorithm::split( data, from, boost::algorithm::is_any_of("-") );// 5-f12a3d4c5e6a7c8af12a3d4c5e6a7c8a
    VERIFY_TRUE(data.size()>=2)<<"invalid version "<<from;
    return strtol(data[0].c_str(), 0, 10);
}
void FileVersionHistory::reload(){
    _current_version = get_current_version_file();
    _doc.clear();
    if(_current_version.filename()=="0-"){
        _doc.put(filter_key("ezsync/modified_time"),0);
        _doc.put(filter_key("ezsync/version"),0 );
        _doc.put(filter_key("ezsync/from"),"0-" );
    }
    else{
        boost::property_tree::read_xml(get_absolute_path(".ezsync/version/history/")+_current_version.filename().string(),_doc);
    }
}
bool FileVersionHistory::append_next_history(boost::property_tree::ptree& doc){
    if(0==get_from_version(doc)) return false;
    log::Debug()<<"append_next_history "<<get_absolute_path(".ezsync/version/history/")+doc.get<std::string>(filter_key("ezsync/from"));
    boost::property_tree::ptree next;
    boost::property_tree::read_xml(get_absolute_path(".ezsync/version/history/")+doc.get<std::string>(filter_key("ezsync/from")),next);
    merge_to(next, doc);
    doc.put(filter_key("ezsync/from"),next.get<std::string>(filter_key("ezsync/from")));
    
    return true;
}
void FileVersionHistory::merge_to(const boost::property_tree::ptree& child, boost::property_tree::ptree& merged) const{
    for(boost::property_tree::ptree::const_iterator it = child.begin(); it != child.end(); it++){
        if(is_leaf(it->second)){
            if(merged.get_child_optional(it->first) == NULL){
                merged.put_child(it->first,it->second);
            }
        }else if(it->second.empty()){
            std::string a = it->first;
            if(merged.get_optional<std::string>(it->first) == NULL){
                merged.put(it->first,it->second.data());
            }
        }else{
            if(merged.get_child_optional(it->first) == NULL){
                merge_to(it->second, merged.put_child(it->first,boost::property_tree::ptree()));
            }else{
                merge_to(it->second, merged.get_child(it->first));
            }
        }
    }
}
bool FileVersionHistory::is_leaf(const boost::property_tree::ptree& child)const{
    boost::optional< const boost::property_tree::ptree& > entry = 
        child.get_child_optional(filter_key("leaf"));
    if(!entry)return false;
    return entry->data() == "1";
}

//预处理文件名,便于作为xml\json等格式的key
boost::property_tree::ptree::path_type FileVersionHistory::filter_key(const std::string& key)const {
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
std::string FileVersionHistory::get_absolute_path(const std::string&path)const{
    return _root.empty()?path:_root+"/"+path;
}
boost::filesystem::path FileVersionHistory::get_current_version_file()const{
    boost::filesystem::path root(get_absolute_path(".ezsync/version/current"));
    if(!boost::filesystem::exists(root)){
        return root/"0-";
    }
    boost::filesystem::path version_file = root/"0-";
    unsigned int max_version = 0;
    for(boost::filesystem::directory_iterator it(root); it != boost::filesystem::directory_iterator(); it++){
        /*if(boost::filesystem::is_directory(*it))*/{
            std::vector<std::string> data;   
            std::string filename = it->path().filename().string();
            boost::algorithm::split( data, filename, boost::is_any_of("-") );// 5-f12a3d4c5e6a7c8af12a3d4c5e6a7c8a
            if(data.size()>=2){
                unsigned int temp = (unsigned int)strtol(data[0].c_str(),0,10);
                if(temp >= max_version){
                    max_version = temp;
                    version_file = *it;
                }
            }
        }
    }
    return version_file;
}
void FileVersionHistory::add_delete_mark(const std::string& key){
    VersionEntry ori = get_entry(key);
    if(ori.is_exist()){ //如果已经在版本中,需要标记为删除
        boost::filesystem::create_directories(get_absolute_path(".ezsync/deleted"));
        std::ostringstream oss;
        oss << key << "$" << ori.version<<"-"<< ori.md5;
        FILE* created = fopen(get_absolute_path(std::string(".ezsync/deleted/")+encode_key(oss.str())).c_str() ,"w");
        VERIFY_TRUE(created != 0)<< "add_delete_mark " << key << "failed with " << errno;
        fclose(created);
    }
}
bool FileVersionHistory::has_delete_mark(const std::string& key,const VersionEntry&ori){
     std::ostringstream oss;
     oss << key << "$" << ori.version<<"-"<< ori.md5;
     return boost::filesystem::exists( get_absolute_path(std::string(".ezsync/deleted/")+encode_key(oss.str())));
}
void FileVersionHistory::clear_delete_marks(){
    boost::filesystem::remove_all(get_absolute_path(".ezsync/deleted/"));
}
std::map<std::string, VersionEntry> FileVersionHistory::get_delete_marks(){
    std::map<std::string, VersionEntry> marks;
    std::map<std::string,Entry> changed;
    boost::filesystem::path path(get_absolute_path(".ezsync/deleted"));
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
void FileVersionHistory::set_custom_value(const std::string& key,const std::string& value)
{
    boost::filesystem::create_directories(get_absolute_path(".ezsync/value"));
    std::ofstream f(get_absolute_path(".ezsync/value/"+key),std::ios::trunc|std::ios::binary);
    f.write(value.c_str(),value.size());
}
std::string FileVersionHistory::get_custom_value(const std::string& key)
{
    std::ifstream f(get_absolute_path(".ezsync/value/"+key),std::ios::binary);
    if(f.is_open()) {
        std::string v;
        f >> v;
        return v;
    }else{
        return "";
    }
}

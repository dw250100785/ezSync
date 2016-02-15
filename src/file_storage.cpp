/**
* $Id$ 
* @author caoyangmin(caoyangmin@gmail.com)
* @brief
*/
#include <sys/stat.h>
#include <iostream>
#include "boost/filesystem.hpp"
#include "ezsync/file_storage.h"
#include "ezsync/verify_true.h"
#include "ezsync/log.h"
using namespace ezsync;

FileStorage::FileStorage(const std::string&root,const std::string &include/* = "*"*/,const std::string &except /*= ""*/)
    :_root(root)
    ,_path_clip(include,except+"\n*.ezsync*"){
       
        while( !_root.empty() && _root[_root.size()-1] == '/'){
            _root = _root.substr(0,_root.size()-1);
        }
}
const std::string& FileStorage::root()const{
    return _root;
}
//void FileStorage::create_entry(const std::string&key){
//    boost::filesystem::path root(get_absolute_path(key));
//    boost::filesystem::create_directories(root.parent_path());
//    FILE* created = fopen(get_absolute_path(key).c_str(),"w");
//    VERIFY_TRUE(created != 0)<< "create_entry " << key << "failed with " << errno;
//    fclose(created);
//}
void FileStorage::del(const std::string& path){
    if(!_path_clip.include(path)) {
        log::Warn()<<"try to delete EXCEPT file " << path;
        return ;
    }

    boost::filesystem::remove_all(get_absolute_path(path).c_str()); // TODO: 文件不存在是失败还是成功?
    
}
Entry FileStorage::get_entry(const std::string& path)const{
    if(!_path_clip.include(path)){
        log::Warn()<<"try to get EXCEPT file " << path;
        return Entry();
    }
    return get_entry_by_path( get_absolute_path(path));
}
std::map<std::string,Entry> FileStorage::get_all_entries(const std::string& path,
    unsigned int modify_time)const{
    //复制过来的文件,如果根据修改时间判断,会导致被排除,无法递交
    modify_time = 0;
    std::map<std::string,Entry> changed;
    boost::filesystem::path root(get_absolute_path(path));
    if(!boost::filesystem::exists(root)) return std::map<std::string,Entry>();
    if(!boost::filesystem::is_directory(root)){
        if(!_path_clip.include(path)) {
            log::Warn() << " skip EXCEPT file : " << path;
            return changed;
        }
        if(boost::filesystem::last_write_time(root) >= modify_time){
              changed[path] = get_entry_by_path(root.string());
        }
        return changed;
    }
    for(boost::filesystem::recursive_directory_iterator it(root); 
        it != boost::filesystem::recursive_directory_iterator(); 
        it++){
        if(!boost::filesystem::is_directory(*it)){
            std::string  relative_path = get_relative_path(*it);
            if(!_path_clip.include(relative_path)) {
                log::Debug() << " skip EXCEPT file : " << relative_path;
                continue ;
            }
            if(boost::filesystem::last_write_time(*it) >= modify_time){
                changed[relative_path] = get_entry_by_path(it->path().string());
            }
        }
    }
    return changed;
}

std::string FileStorage::get_absolute_path(const std::string&path)const{
    return _root.empty()?path:_root+"/"+path;
}
//获取相对路径
//刚好能工作,不可靠的算法...
std::string FileStorage::get_relative_path(const boost::filesystem::path& abs_path)const{
    boost::filesystem::path root(_root);
    boost::filesystem::path::iterator x = root.begin();
    boost::filesystem::path::iterator y = abs_path.begin();
    boost::filesystem::path res;
    while(y != abs_path.end()){
        if(x ==  root.end()){
            res/=*y;
        }else{
            x++;
            if(*x == ".") continue;
        }
        y++;
    }
    return res.string();
}
Entry FileStorage::get_entry_by_path(const std::string& path)const{
    struct  stat info = {0};
    int r = stat(path.c_str(),&info);
    VERIFY_TRUE(r==0 || (r!=0 && errno==ENOENT))<< "errno="<<errno;
    if(r==0){
        Entry entry;
        entry.content = path;
        entry.md5 = get_file_md5(path);
        entry.modified_time= (unsigned int)info.st_mtime;
        entry.size = info.st_size;
        return entry;
    }
    else{
        return Entry();
    }
}

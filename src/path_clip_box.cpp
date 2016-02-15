/**
* $Id: path_clip_box.cpp 607 2014-03-06 14:24:32Z caoyangmin $ 
* @author caoyangmin(caoyangmin@gmail.com)
* @brief PathClipBox
*/
#include <boost/algorithm/string.hpp>
#include "ezsync/path_clip_box.h"

using namespace ezsync;
PathClipBox::PathClipBox(const std::string&include,const std::string& except){
    std::list<std::string>  in;
    boost::algorithm::split( in, include, boost::is_any_of("\n") );
    std::list<std::string>  ex;
    boost::algorithm::split( ex, except, boost::is_any_of("\n") );

    get_clips(in,_includes);
    get_clips(ex,_excepts);
}
/**
* @param include 包含
* @param except 排除
*/
PathClipBox::PathClipBox(const std::list<std::string>& include, const std::list<std::string>& except){
    get_clips(include,_includes);
    get_clips(except,_excepts);
}
/**判断是否包含*/
bool PathClipBox::include(const std::string&path)const{
    //判断是否被包含
    if(!_includes.empty()){
        if(!std::any_of(_includes.begin(),_includes.end(),std::bind(is_include,std::placeholders::_1,path))){
            return false;
        }
    }
    //判断是否被排除except
    return !std::any_of(_excepts.begin(),_excepts.end(),std::bind(is_include,std::placeholders::_1,path));
}


bool PathClipBox::is_include(const Clip& clip , const std::string& path){
    size_t pos = 0;
    bool find_from_any = false;
    for(Clip::const_iterator it = clip.begin(); it != clip.end(); it++){
        if(it->empty()) {
            find_from_any = true;
            continue;
        }
        if(find_from_any){
            find_from_any = false;
            pos = path.find(*it,pos);
            if(pos == std::string::npos) return false;
        }else{
            if(pos != path.find(*it,pos)) return false;
            find_from_any = true;
        }
        pos += it->size();
    }
    if(!find_from_any){//如果最后不是*,则最后字符必须严格匹配
        return pos == path.size();
    }
    return  true;
}
void PathClipBox::get_clips(const std::list<std::string>&strs, std::list<Clip>& clips){
    for(std::list<std::string>::const_iterator it = strs.begin();it!=strs.end();it++){
        if(it->empty()) continue;
        Clip clip;   
        //TODO: 普通路径后面增加\n/*,变量文件完全匹配+路径匹配
        if(it->at(it->size()-1) == '/' ){ //路径即包含路径下所有文件
            std::string str = *it + "*";
            boost::algorithm::split( clip, str, boost::is_any_of("*") );
        }else{
            boost::algorithm::split( clip, *it, boost::is_any_of("*") );
        }
        clips.push_back(clip);
    }
}

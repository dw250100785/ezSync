/**
* $Id: path_clip_box.h 607 2014-03-06 14:24:32Z caoyangmin $ 
* @author caoyangmin(caoyangmin@gmail.com)
* @brief
*/
#pragma once
#include <list>
#include <string>
#include <algorithm>
#include <vector>
#include "utility.h"

namespace ezsync{
    /** 
     *路径包含\排除处理
     */
    class PathClipBox
    {
    public:
        PathClipBox(const std::string&include = "*",const std::string& except="");
        /**
         * @param include 包含
         * @param except 排除
         */
        PathClipBox(const std::list<std::string>& include, const std::list<std::string>& except);
        /**判断是否包含*/
        bool include(const std::string&path)const;
    private:
        
        typedef std::vector<std::string> Clip;
        std::list<Clip> _includes;
        std::list<Clip> _excepts;
     
        static bool is_include(const Clip& clip , const std::string& path);
        void get_clips(const std::list<std::string>&strs, std::list<Clip>& clips);
    };
}

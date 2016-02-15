/**
* $Id: transfer.h 717 2014-03-26 03:39:28Z caoyangmin $ 
* @author caoyangmin(caoyangmin@gmail.com)
* @brief
*/
#pragma once
#include <string>
#include <tuple>
#include <vector>
#include "ezsync/utility.h"

namespace ezsync{
    struct Entry;
    namespace transfer
    {
  
        typedef std::vector< std::tuple<std::string,Entry,std::string> > CopyItems ;

        template<class FROM,class TO>
        void copy(FROM& from, const std::string& from_path,const Entry& src,TO& to, const std::string& to_path);

        template<class FROM,class TO>
        void muti_copy(FROM& from,TO& to, const CopyItems& items);

    }
}
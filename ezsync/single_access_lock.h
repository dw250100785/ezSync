/**
* $Id: single_access_lock.h 961 2014-06-12 06:55:56Z caoyangmin $ 
* @author caoyangmin(caoyangmin@gmail.com)
* @brief
*/
#pragma once
#include <sstream>
#include <fstream>
#include "boost/filesystem.hpp"
#include <sys/types.h>
#ifndef WIN32
#include <unistd.h>
#else
#include "windows.h"
#define getpid GetCurrentProcessId
#endif
#include "ezsync/log.h"
#include "ezsync/verify_true.h"

namespace ezsync{
    struct SigleAccessLockException:std::exception{};
    class SigleAccessLock
    {
    public:
        SigleAccessLock(const std::string& path)
        :_path(path + "/.ezsync/.lock"){
                std::ostringstream oss;
                oss << _path << "/"<<getpid();
                boost::filesystem::create_directories(_path);
                std::ofstream f(_path + "/unknown",std::ios::binary);
                f.close();

                try{
                	boost::filesystem::rename(_path + "/unknown",oss.str());
                }catch(const std::exception &e){
                	VERIFY_TRUE_THROW(false,SigleAccessLockException)<<" lock failed with " << e.what() <<": "<< path;
                }
                log::Debug()<<"lock " << oss.str() << " OK";

        }
        ~SigleAccessLock(){
            try{
                boost::filesystem::remove_all(_path);
                log::Debug()<<"unlock " << _path << " OK";
            }catch(const std::exception &e){
                log::Error()<<"unlock " << _path << " failed with " << e.what();
            }
            
        }
    private:
        std::string _path;
    };
}//namespace ezsync

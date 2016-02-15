/**
 * $Id: verify_true.h 970 2014-06-25 09:51:39Z caoyangmin $
 * @author caoyangmin(caoyangmin@gmail.com)
 **/
#ifndef __VERIFY_TRUE_H__
#define __VERIFY_TRUE_H__
#include <stdexcept>
#include <sstream>
#include <cstdlib>
#include "ezsync/log.h"

#ifndef VERIFY_TRUE
template<class T=std::exception>
struct VerifyTrueHelper: T{
	VerifyTrueHelper(bool log=false)
        :_log(log){
    }
    const char* what() const throw(){
        return _what.c_str();
    }
    std::string _what;
    bool _log;
    template<class I>
    VerifyTrueHelper& operator <<(const I&t){
        std::ostringstream oss;
        oss << t;
        _what.append(oss.str());
        return *this;
    }
    VerifyTrueHelper(const VerifyTrueHelper&rh){
        *this = rh;
        _log = false;//日志只在最初的异常中打印
    }
    VerifyTrueHelper& operator = (const VerifyTrueHelper&rh){
        _what = rh._what;
        return *this;
    }
    virtual ~VerifyTrueHelper()throw(){
        if(_log) ezsync::log::Warn()<<_what;
    }
};

#define VERIFY_TRUE(var)\
    (var)?true:throw VerifyTrueHelper<>(true)<<"@"<<__FUNCTION__<<":"<<__LINE__<<"@\t"

#define VERIFY_TRUE_THROW(var,T)\
    (var)?true:throw VerifyTrueHelper<T>(true) <<"@"<<__FUNCTION__<<":"<<__LINE__<<"@\t"<< #T"\t"

#endif //VERIFY_TRUE

#endif //__VERIFY_TRUE_H__

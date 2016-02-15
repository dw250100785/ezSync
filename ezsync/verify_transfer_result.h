/**
* $Id: verify_transfer_result.h 970 2014-06-25 09:51:39Z caoyangmin $ 
* @author caoyangmin(caoyangmin@gmail.com)
* @brief
*/
#pragma once
#include "ezsync/verify_true.h"
#include "ezsync/TransferInterface.h"
namespace ezsync{
    class AuthFailException:public std::exception{};

    #define VERIFY_TRANSFER_RESULT(fun,error)\
    if(!(fun)){\
        switch(error.error_code){\
        case AUTH_FAILED:\
            throw VerifyTrueHelper<AuthFailException>(true)<<"@"<<__FUNCTION__<<":"<<__LINE__<<"@\t" << error.error_code << " " << error.error_msg;\
        default:\
            throw VerifyTrueHelper<>(true)<<"@"<<__FUNCTION__<<":"<<__LINE__<<"@\t" << error.error_code << " " << error.error_msg;\
        }\
    }
}
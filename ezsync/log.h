/**
* $Id: log.h 550 2014-02-27 15:00:40Z caoyangmin $ 
* @author caoyangmin(caoyangmin@gmail.com)
* @brief
*/
#pragma once
#include <string>
#include <sstream>
namespace ezsync{
    namespace log
    {
        struct Out;
       
        //typedef  Out& (*Endl)(Out&)  ;
        void print_log(const std::string& tag, const std::string& str);
    
        struct Out{
            Out(const std::string& tag):tag(tag){
            }
            template<class I>
            Out& operator <<( const I& t){
                oss << t;
                return *this;
            }
            
            /*template<>
            Out& operator<< <Endl> (const Endl& call){
                call(*this);
                return *this;
            }*/
            std::ostringstream oss;
            const std::string tag;
            ~Out(){
                print_log(tag,oss.str());
            }
        };
        inline Out& endl(Out& out){
            out << "\r\n";
            return out;
        }
        
        struct Debug :Out{
            Debug():Out("debug"){
                
            }
        };
        struct Info :Out{
            Info():Out("info"){
                
            }
        };
        struct Warn :Out{
            Warn():Out("warn"){
                
            }
        };
        struct Error :Out{
            Error():Out("error"){
                
            }
        };
    }
}
/**
* $Id$ 
* @author caoyangmin(caoyangmin@gmail.com)
* @brief
*/
#include <stdio.h>
#include <errno.h>
#include "md5.h"
#include "ezsync/utility.h"
#include "ezsync/verify_true.h"
using namespace ezsync;
namespace ezsync{
    std::string get_text_md5(const std::string& text){
        md5_state_t md5;
        md5_init(&md5);
        md5_append(&md5, (md5_byte_t*)text.c_str(), text.size());
        md5_byte_t res[16];
        md5_finish(&md5,res);
        char res_str[sizeof(res)*2+1]={0};
        for(size_t i=0; i < sizeof(res); i++){
            sprintf(res_str + i*2,"%02x",res[i]);
        }
        return res_str; 
    }
    std::string get_file_md5(const std::string& path){
        FILE* f = fopen(path.c_str(),"rb");
        VERIFY_TRUE(f != 0) << "open " << path << "failed with "<<errno;
        md5_byte_t buffer[4096];
        size_t readed = 0;
        md5_state_t md5;
        md5_init(&md5);
        while(readed = fread(&buffer,1,sizeof(buffer),f)){
            md5_append(&md5,buffer,readed);
        }
        fclose(f);

        md5_byte_t res[16];
        md5_finish(&md5,res);
        char res_str[sizeof(res)*2+1]={0};
        for(size_t i=0; i < sizeof(res); i++){
            sprintf(res_str + i*2,"%02x",res[i]);
        }
        return res_str; 
    }

    //自定义编码:初字母数字外的所有字符转成 _+十六进制
    std::string encode_key(const std::string& key,char sap/*='_'*/){
        std::string to;
        for(std::string::const_iterator it=key.begin(); it!=key.end(); it++){
            if(isdigit((unsigned char)*it) || isalpha((unsigned char)*it)) to.push_back((unsigned char)*it);
            else {
                to.push_back(sap);
                char str[32]= {0};
                sprintf(str,"%02x",(unsigned char)*it);
                to.append(str);
            }
        }
        return to;
    }
    std::string decode_key(const std::string& key,char sap/*='_'*/){
        std::string to;
        for(size_t i=0; i<key.size(); i++){
            if(key[i] == sap){
                to.push_back((char)strtol(key.substr(i+1,2).c_str(),0,16));
                i+=2;
            }else{
                to.push_back(key[i]);
            }
        }
        return to;
    }
}
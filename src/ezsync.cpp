/**
* $Id$ 
* @author caoyangmin(caoyangmin@gmail.com)
* @brief
*/

#include <iostream>
#include "boost/program_options.hpp"
#include "ezsync/update_method2.h"
#include "ezsync/delete_method2.h"
#include "ezsync/commit_method2.h"
#include "ezsync/file_storage.h"
#include "ezsync/cloud_storage.h"
#include "ezsync/sqlite_storage.h"
#include "ezsync/TransferInterface.h"
#include "ezsync/file_sync_client_builder2.h"
#include "ezsync/sqlite_sync_client_builder2.h"
#include "curl/curl.h"
#ifdef WIN32
#include "Windows.h"
#endif
#include "ezsync/log.h"
#include "ezsync/path_clip_box.h"
#include "android/jni/ezsync_client_wrap.h"
#include "boost/property_tree/ptree.hpp"
#include "boost/property_tree/json_parser.hpp"
//#include "ezsync/mix_storage.h"
//TODO: 下载时如果缺少文件,忽略?
//TODO: 维护已上传成功的文件列表,防止再次上传?

namespace po = boost::program_options;

using namespace ezsync;
UseWitch merge_test(const std::string&key,const Entry&,const Entry&){
    std::cout << key << " merged:use_local"<<std::endl;
    return USE_LOCAL;
}

class UseLocal:public IEzsyncMergeStrategy{
public:
    ezsync::UseWitch merge(const std::string& key,const ezsync::Entry&,const ezsync::Entry&){
        std::cout << key << " merged:use_local"<<std::endl;
        return USE_LOCAL;
    }
}_use_local;

namespace ezsync{
    namespace log
    {
       void print_log(const std::string& tag,const std::string& str){
           printf("--%s--\t%s\r\n",tag.c_str(),str.c_str());
       }
    }
}
void conflicting_options(const po::variables_map& vm, 
                         const char* opt){
    if (!vm.count(opt) && !vm[opt].defaulted() )
        throw std::logic_error(std::string("miss option: ") + opt);
}

//sqlite数据库文件同步配置
//{"local_type":"sqlite","host":"127.0.0.1:443","path":"/"}
//文件同步设置
int main(int argc, char* argv[]){
    try {
        po::options_description desc("Allowed options");

        desc.add_options()
            ("help,h", "print usage message")
            ("operator,o", po::value<std::string>(), "up/ci/del")
            ("local,l", po::value<std::string>(), "local path")
            ("remote,r", po::value<std::string>(), "remote path")
            ("conf,c", po::value<std::string>(), "config file")
            ("include,i", po::value<std::string>()->default_value(""), "include,used by operator = up")
            ("except,x", po::value<std::string>()->default_value(""), "except,used by operator = up")
            ("file,f", po::value<std::string>(), "file,used by operator = del")
        ;
        po::positional_options_description p;
        p.add("operator", -1);

        po::variables_map vm;
        po::store(po::command_line_parser(argc, argv).
                  options(desc).positional(p).run(), vm);
        po::notify(vm); 

        if (vm.count("help")) {
            std::cout << desc << "\n";
            return 1;
        }
        conflicting_options(vm,"operator");
        conflicting_options(vm,"conf");
        conflicting_options(vm,"local");
        conflicting_options(vm,"remote");

        boost::property_tree::ptree doc;
        boost::property_tree::json_parser::read_json(vm["conf"].as<std::string>(),doc);
        std::map<std::string, std::string> map ;
        for(boost::property_tree::ptree::const_iterator it = doc.begin();
            it != doc.end(); 
            it++){
                map[it->first] = it->second.get_value("");
        }
        std::shared_ptr<IEzsyncClientWrap> client;
   
        map["local_storage"] = vm["local"].as<std::string>();
        map["remote_storage"] = vm["remote"].as<std::string>();

        map["local_history"] = vm["local"].as<std::string>();
        map["remote_history"] = vm["remote"].as<std::string>();

		if (map["local_type"] == "file") {
            client.reset(new EzsyncClientWrap<ezsync::FileSyncClient2>(ezsync::FileSyncClientBuilder2::build(map)));

		} else if (map["local_type"] == "sqlite") {
			client.reset(new EzsyncClientWrap<ezsync::SqliteSyncClient2>(ezsync::SqliteSyncClientBuilder2::build(map)));
		} else {
			throw std::logic_error(std::string("unknown local_type"));
		}
        if (vm.count("operator")) {
           if(vm["operator"].as<std::string>() == "up"){
			   //更新
               client->update(_use_local,
                   vm["include"].as<std::string>() , vm["except"].as<std::string>() ,NULL);

           }else if(vm["operator"].as<std::string>() == "ci"){
			   //递交
               client->commit("");

           }if(vm["operator"].as<std::string>() == "del"){
			   //删除
               client->del("");
           }
        } 
        std::cout << "ok\n";
        return 0;
    }
    catch(const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return -1;
    }
    std::cout << "use help or -h"<< "\n";

    return 0;

}


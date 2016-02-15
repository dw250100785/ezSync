/**
 * @file TransferInterface.h
 * @author yangxugang
 * @brief 
 *     本文件主要定义了TransferInterface
 **
*/

#ifndef TRANSFER_INTERFACE_H
#define TRANSFER_INTERFACE_H

#include <map>
#include <string>
#include <list>
#include <vector>
#include <utility>

namespace ezsync{

//请求header 对
typedef std::list<std::pair<std::string, std::string> > headers_t;

enum error_code_t 
{
	INIT_TRANSFER_FAILED = 10, //transfer 初始化失败
    NET_ERROR,//网络错误
	CANCELED,//已经取消
	DF_FULL,//磁盘满
	PARAM_EMPTY,//参数为空
    UPLOAD_FILE_OPEN_FAILED,//上传打开文件失败
    DOWNLOAD_FILE_OPEN_FAILED,//下载打开文件失败
    PARSE_RESPONSE_FAILED,//解析返回错误信息失败
    AUTH_FAILED,//鉴权失败

	FILE_EXIST,//文件已经存在
	FILE_NOT_EXIST,//文件不存在

	SERVER_REQUEST_ERROR,//请求错误
	SERVER_INTERNAL_ERROR,//服务内部错误

	BATCH_PROTOCOL_PARSE_FAILED,//批量协议解析错误
	BATCH_SERVER_TO_TARGET_SERVER_FAILED//批量服务到目标服务错误
};

struct error_info_t
{
public:
	error_info_t():
		error_code(0){}
		
	int error_code;
	std::string error_msg;
};

struct return_info_t
{
public:
	return_info_t():
		is_success(false){}
		
	bool is_success;
	error_info_t error_info;
};

struct meta_info_t
{
public:
	meta_info_t()
		:mtime(0),size(0),isdir(false){}

	std::string md5;
	int mtime;
	int size;
    bool isdir;
};

struct enum_item_t
{
public:
	meta_info_t meta;
	std::string path;
};


class TransferInterface
{
public:
	TransferInterface(){}
	virtual ~TransferInterface(){}
public:
	virtual bool makedir(const std::string& dir, error_info_t& error_info) = 0;
    virtual bool upload_from_file(const std::string& from, const std::string& to, bool is_overwrite, error_info_t& error_info) = 0;
	virtual bool upload_from_buffer(const std::string& from, const std::string& to, bool is_overwrite, error_info_t& error_info) = 0;
    virtual bool download_to_file(const std::string& from, const std::string& to, error_info_t& error_info) = 0;
	virtual bool download_to_buffer(const std::string& from, std::string& to, error_info_t& error_info) = 0;
    virtual bool list(const std::string& dir, std::list<enum_item_t>& enum_list, error_info_t& error_info) = 0;
	virtual bool del(const std::string& path, error_info_t& error_info) = 0;
	virtual bool move(const std::string& from, const std::string& to, error_info_t& error_info) = 0;
	virtual bool meta(const std::string& path, meta_info_t& meta_info, error_info_t& error_info) = 0;
	virtual void get_process(double& process_now, double& process_total) = 0;
	virtual void cancel() = 0;
    virtual void reset() = 0;
	virtual void set_cookie(const std::string& cookie) = 0;

	//batch
	virtual bool batch_download_to_file(const std::vector<std::pair<std::string, std::string> >& request_list, std::vector<return_info_t>& return_list, error_info_t& error_info) = 0;
	virtual bool batch_download_to_buffer(std::vector<std::pair<std::string, std::string> >& request_list, std::vector<return_info_t>& return_list, error_info_t& error_info) = 0;
	virtual bool batch_upload_from_file(const std::vector<std::pair<std::string, std::string> >& request_list, bool is_overwrite, std::vector<return_info_t>& return_list, error_info_t& error_info) = 0;
	virtual bool batch_upload_from_buffer(const std::vector<std::pair<std::string, std::string> >& request_list, bool is_overwrite, std::vector<return_info_t>& return_list, error_info_t& error_info) = 0;

	virtual void set_ssl_is_verify_ca(bool is_verify) = 0;
	virtual void set_ssl_is_verify_host(bool is_verify) = 0;
	virtual void set_ssl_ca_file(const std::string& ca_file) = 0;
	virtual void set_ssl_ca_buffer(const std::string& ca_buffer) = 0;
	
};

}//namespace ezsync

#endif

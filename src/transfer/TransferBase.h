/**
 * @file TransferBase.h
 * @author yangxugang
 * @brief 
 *     本文件主要定义了TransferBase类
 **/
#ifndef TRANSFER_BASE_H
#define TRANSFER_BASE_H

#include <vector>

#include <signal.h>

#include <openssl/ssl.h>
#include "curl/curl.h"
#include "rapidjson/document.h"
//#include "c-ares/ares.h"

#ifdef ANDROID
#include <jni.h>
#endif

#include "ezsync/TransferInterface.h"
#include "CurlShare.h"

namespace ezsync{

class TransferBase;
#define HTTP_RET_CODE_SUCCESS 200
#define HTTP_RET_CODE_CONTINUE_FAILED 100
#define BATCH_RESULT_INIT_VALUE -1000

//curl handle count
#define CURL_HANDLE_POOL_SIZE 3

//curl 超时单位秒
#define CURL_TIMEOUT_SECOND 20

//try count
#define REQUEST_TRY_COUNT 1

enum ctl_t
{
	CTL_CANCEL,//取消运行
	CTL_RUN//运行
};

enum request_type_t
{
	UPLOAD_FROM_FILE,
	UPLOAD_FROM_BUFFER,
	DOWNLOAD_TO_FILE,
	DOWNLOAD_TO_BUFFER,
	MKDIR,
	LIST,
	DEL,
	MOVE,
	META,
	BATCH_DOWNLOAD,
	BATCH_UPLOAD
};

struct down_write_param_t
{
public:
    down_write_param_t():
        transfer(NULL), to_stream(NULL), error_stream(NULL), curl_handle(NULL)
    {
    }
    TransferBase* transfer;
    void* to_stream;
	std::string* error_stream;
	CURL* curl_handle;
};

struct read_buf_arg_t
{
public:
	read_buf_arg_t():
		buf(NULL),len(0),pos(0){}
	char *buf;
	int len;
	int pos;
};

struct response_info_t
{
public:
	response_info_t()
		:to_file_handle(NULL),is_http_header_finished(false),is_http_body_finished(false),
		http_code(0),batch_result(BATCH_RESULT_INIT_VALUE),http_header_total(0),http_header_now(0),
		http_body_total(0),http_body_now(0)
		{}
	
public:
	FILE* to_file_handle;
	std::string to_file_name;
	std::string http_header_buf;
	std::string http_body_buf;
	std::map<std::string, std::string> http_headers;
	error_info_t* error_info;
	int http_code;
	int batch_result;
	int http_header_total;
	int http_header_now;
	int http_body_total;
	int http_body_now;
	bool is_http_header_finished;
	bool is_http_body_finished;
};

enum batch_next_t
{
	BATCH_RECV_PROTOCOL_HEAD,
	BATCH_RECV_HTTP_HEAD,
	BATCH_RECV_HTTP_BODY
};

struct batch_info_t
{
public:
	batch_info_t()
		:batch_next(BATCH_RECV_PROTOCOL_HEAD),batch_list_size(0),current_seq(-1),
		is_can_retry(true),error_info(NULL),curl_handle(NULL){}

	std::map<int, response_info_t> batch_response_info;
	std::string request_buf;//请求buf 
	std::string protocol_head_buf;//协议头buffer
	batch_next_t batch_next;//下一步接收什么数据
	int batch_list_size;//批量请求个数
	int current_seq;//当前处理的序号
	bool is_can_retry;//连接批量代理失败的情况下可以重试已经收到部分数据不可以重试

	std::string* error_stream;//批量请求数据通道返回信息
	error_info_t* error_info;
	CURL* curl_handle;
};


enum batch_send_next_t
{
	BATCH_SEND_PROTOCOL_HEAD,
	BATCH_SEND_HTTP_BODY
};

struct upload_info_t
{
public:
	upload_info_t()
		:http_body_buf(NULL),from_file_handle(NULL),protocol_head_total(0),protocol_head_now(0),
		http_body_total(0),http_body_now(0)
		{}

	std::string protocol_head_buf;
	const std::string* http_body_buf;
	FILE* from_file_handle;
	int protocol_head_total;
	int protocol_head_now;
	int http_body_total;
	int http_body_now;
};

struct batch_upload_info_t
{
public:
	batch_upload_info_t()
		:batch_send_next(BATCH_SEND_PROTOCOL_HEAD),batch_list_size(0),current_seq(0),request_len(0)
		{}

	batch_send_next_t batch_send_next;
	std::map<int, upload_info_t> batch_upload_info;
	int batch_list_size;
	int current_seq;
	int request_len;
};

struct request_para_t 
{
public:
	request_para_t()
		:error_info(NULL),batch_info(NULL),batch_upload_info(NULL)
	{

	}

	std::string request_url;
	request_type_t request_type;
	std::string from;//
	std::string to;//
	std::string custom_request;//考虑到pcs因此需要传入http method
	std::map<std::string, std::string> response_headers;
	std::string response;//请求返回如果下载到buffer则为目标buffer
	error_info_t* error_info;//
	batch_info_t* batch_info;
	batch_upload_info_t* batch_upload_info;
};

//
struct request_info_t
{
public:
	request_info_t()
		:formpost(NULL),lastptr(NULL),from_file(NULL),to_file(NULL),
		headers(NULL),upload_file_size(0),is_put_upload(false),
		http_retcode(0),is_curl_error(false),is_df_full(false),
		request_seq(-1),retry_count(0),curl_handle(NULL),
		is_add_request_succes(false),is_request_success(false),
		process_now(0),process_total(0)
	{
		memset(curl_error_msg, 0, CURL_ERROR_SIZE);
	}

	void reset()
	{
		//request_para = NULL;
		formpost = NULL;
		lastptr = NULL;
		from_file = NULL;
		headers = NULL;
		read_buf_arg.buf = NULL;
		read_buf_arg.len = 0;
		read_buf_arg.pos = 0;
		upload_file_size = 0;
		is_put_upload = false;
		http_retcode = 0;
		is_curl_error = false;
		is_df_full = false;
		memset(curl_error_msg, 0, CURL_ERROR_SIZE);
		//request_seq = -1;
		//retry_count = 0;
		//curl_handle = NULL;
		is_add_request_succes = false;
		is_request_success = false;
		process_now = 0;
		process_total = 0;
	}

	request_para_t request_para;//请求所需参数

	struct curl_httppost *formpost;
	struct curl_httppost *lastptr;
	FILE *from_file;
	FILE *to_file;
	struct curl_slist * headers;
	read_buf_arg_t read_buf_arg;
	long upload_file_size;
	bool is_put_upload;
	long http_retcode;
	bool is_curl_error;//是否有curl错误
	bool is_df_full;//磁盘是否写满
	char curl_error_msg[CURL_ERROR_SIZE];

	int request_seq;//初始化后始终不变即重试也不变
	int retry_count;
	CURL* curl_handle;//重试后不能改变
	
	bool is_add_request_succes;//添加请求是否成功
	bool is_request_success;//请求是否执行成功

	double process_now;//当前下载或上传量
	double process_total;//当前下载或上传总量
};

class TransferBase : public TransferInterface
{
public:
    TransferBase(const std::string& domain, const std::string& root_path
		, headers_t headers, const std::string& cookie_path, bool use_ssl);
    virtual ~TransferBase();
	
protected:
	virtual void get_error_info(long http_retcode, const std::string& response, error_info_t& error_info) = 0;
	
public:
	void set_cookie(const std::string& cookie);
	void set_ssl_is_verify_ca(bool is_verify);
	void set_ssl_is_verify_host(bool is_verify);
	void set_ssl_ca_file(const std::string& ca_file);
	void set_ssl_ca_buffer(const std::string& ca_buffer);

	void get_process(double& process_now, double& process_total);
	void cancel();
    void reset();

	std::string gethostbyname(const std::string& domain);
	std::string get_resolved_url(const std::string& request_url);
#ifdef ANDROID
	static void set_jvm(JavaVM* jvm);
#endif
protected:
	static int handle_batch_read(char *send_buf, size_t send_buf_len, batch_upload_info_t* batch_upload_info);
	static int batch_read_buffer(char *send_buf, size_t size, size_t nitems, batch_upload_info_t* batch_upload_info);
	static bool parse_batch_http_header(batch_info_t* batch_info);
	static bool parse_batch_protocol_head(batch_info_t* batch_info);
	static int handle_batch_recv(char* data, size_t data_len, batch_info_t* batch_info);
	static int batch_response_write_buffer(char *data, size_t size, size_t nmemb, batch_info_t* batch_info);
	static int response_write_buffer(char *data, size_t size, size_t nmemb, std::string *buffer);
	static int download_write_file(void *ptr, size_t size, size_t nmemb, void *params);
	static int header_handler(void *ptr, size_t size, size_t nmemb, void *userdata) ;
	static int upload_process(void* clientp, double dltotal, double dlnow, double ultotal, double ulnow);
    static int download_process(void* clientp, double dltotal, double dlnow, double ultotal, double ulnow);
	static int read_buffer(void *ptr, size_t size, size_t nitems, void *stream);	
	static CURLcode sslctx_function(CURL * curl, void * sslctx, void * parm);
	unsigned char to_hex(unsigned char x);
	std::string url_encode(const std::string& str);
	static void split(std::string str,std::string pattern, std::vector<std::string>& result);
	void set_ssl(CURL* curl_handle);
	bool request(
		const std::string& request_url, request_type_t request_type, 
		const std::string& from, const std::string& to, const std::string&custom_request, 
		std::map<std::string, std::string>* response_headers, std::string& response,
		error_info_t& error_info, batch_info_t* batch_info = NULL, batch_upload_info_t* batch_upload_info = NULL);

	//
	static int upload_process_handle(request_info_t* request_info, double dltotal, double dlnow, double ultotal, double ulnow);
	static int download_process_handle(request_info_t* request_info, double dltotal, double dlnow, double ultotal, double ulnow);
	static int download_write_file_handle(void *ptr, size_t size, size_t nmemb, request_info_t* request_info);

	bool add_request(request_info_t& request_info);
	bool perform_request(request_info_t request_info_pool[CURL_HANDLE_POOL_SIZE]);
	bool get_perform_request_result(ctl_t ctl_run_status, int still_running, request_info_t request_info_pool[CURL_HANDLE_POOL_SIZE]);
	bool retry_request(request_info_t request_info_pool[CURL_HANDLE_POOL_SIZE]);

	bool set_curl_opt(request_info_t& request_info);
	bool set_common_curl_opt(request_info_t& request_info);
	bool set_upload_from_file_curl_opt(request_info_t& request_info);
	bool set_upload_from_buffer_curl_opt(request_info_t& request_info);
	bool set_download_to_file_curl_opt(request_info_t& request_info);
	bool set_download_to_buffer_curl_opt(request_info_t& request_info);
	bool set_move_curl_opt(request_info_t& request_info);
	bool set_meta_curl_opt(request_info_t& request_info);
	bool set_batch_download_curl_opt(request_info_t& request_info);
	bool set_batch_upload_curl_opt(request_info_t& request_info);

	//
	//void get_dns_server_list(std::string& dns_server_list);

protected:
	headers_t _headers;//请求头信息
	std::string _cookie_path;//cookie保存路径
	std::string _domain;//服务域名
	std::string _root_path;//服务端的根路径如/apps/pcstest_oauth 注意最后没有/
	std::string _cookie;//保存设置的cookie

	bool _use_ssl;
	bool _is_verify_ca;//ssl是否验证CA
	bool _is_verify_host;// 检查证书中是否设置域名，并且是否与提供的主机名匹配 
	std::string _ca_file;//ssl ca 文件路径
	std::string _ca_buffer;//ssl ca buffer
	
    double _process_now;//当前下载或上传量
    double _process_total;//当前下载或上传总量
    ctl_t _ctl_run;
    bool _df_full;
	CURL *_curl_handle;
	CURLM *_multi_handle;
	int _try_count;
	static CurlShare _curl_share;//curl share 
	
	CURL *_curl_handle_pool[CURL_HANDLE_POOL_SIZE];

private:
#ifdef ANDROID
	static JavaVM* _s_jvm;
#endif
};
 

}//namespace ezsync

#endif//TRANSFER_BASE_H

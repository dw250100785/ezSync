/**
 * @file TransferBase.cpp
 * @author yangxugang
 * @brief 
 *     本文件主要定义了TransferBase类实现
 **/
#include <algorithm>
#ifndef WIN32
#include <arpa/inet.h>
#endif

#include "TransferBase.h"
#include "ezsync/log.h"

namespace ezsync{

//#ifdef WIN32
//	namespace log
//	{
//		void print_log(const std::string& tag,const std::string& str){
//			printf("--%s--\t%s\r\n",tag.c_str(),str.c_str());
//		}
//	}
//#endif

CurlShare TransferBase::_curl_share;

#ifdef ANDROID
JavaVM* TransferBase::_s_jvm = NULL;
#endif

TransferBase::TransferBase(const std::string& domain, const std::string& root_path
	, headers_t headers, const std::string& cookie_path, bool use_ssl):
	_domain(domain),_root_path(root_path), _headers(headers), _cookie_path(cookie_path),
	_use_ssl(use_ssl),_is_verify_ca(false),_is_verify_host(false),
	_process_now(0),_process_total(0),_ctl_run(CTL_RUN),_df_full(false),_try_count(0)
{
	//去除路径后面的/
	std::size_t pos = _root_path.rfind("/");
	if((std::string::npos != pos) && (_root_path.length()-1) == pos)
	{
		_root_path.replace(pos, 1, "");
	}
#ifdef ANDROID
	signal(SIGPIPE, SIG_IGN);
#endif
	
	//curl pool
	for (int i = 0; i < CURL_HANDLE_POOL_SIZE; i++)
	{
		_curl_handle_pool[i] = curl_easy_init();
	}

	_curl_handle = curl_easy_init();
    _multi_handle = curl_multi_init();

	//start the cookie engine
	if(_curl_handle)
	{
		curl_easy_setopt(_curl_handle, CURLOPT_COOKIEFILE, _cookie_path.c_str());
		curl_easy_setopt(_curl_handle, CURLOPT_COOKIEJAR, _cookie_path.c_str());
	}
	
}

TransferBase::~TransferBase()
{
	curl_multi_cleanup(_multi_handle);
    curl_easy_cleanup(_curl_handle);

	//curl pool
	for (int i = 0; i < CURL_HANDLE_POOL_SIZE; i++)
	{
		curl_easy_cleanup(_curl_handle_pool[i]);
	}
}
/*
void TransferBase::get_dns_server_list(std::string& dns_server_list)
{
	ares_channel channel = NULL;
	struct ares_addr_node *servers = NULL;

	int status = ares_library_init(ARES_LIB_INIT_ALL);
	if (status == ARES_SUCCESS)
	{
		status = ares_init(&channel);
		if (status == ARES_SUCCESS)
		{
			status = ares_get_servers(channel, &servers);
			if (status == ARES_SUCCESS)
			{
				int i = 0;
				struct ares_addr_node *srvr;
				for (i = 0, srvr = servers; srvr; i++, srvr = srvr->next)
				{
					if (srvr->family == AF_INET)
					{
						char *ip = inet_ntoa(srvr->addr.addr4);
						if (NULL != ip)
						{
							if (!dns_server_list.empty())
							{
								dns_server_list.append(",");
							}

							dns_server_list.append(ip);
						}
					}
				}
			}

		}
	}

	ares_free_data(servers);
	ares_destroy(channel);
	ares_library_cleanup();


	if (!dns_server_list.empty())
	{
		dns_server_list.append(",");
	}
	dns_server_list.append("8.8.8.8,8.8.4.4");
}
*/
void TransferBase::set_cookie(const std::string& cookie)
{
	_cookie = cookie;
}

void TransferBase::set_ssl_is_verify_ca(bool is_verify)
{
	_is_verify_ca = is_verify;
}

void TransferBase::set_ssl_is_verify_host(bool is_verify)
{
	_is_verify_host = is_verify;
}
	
void TransferBase::set_ssl_ca_file(const std::string& ca_file)
{
	_ca_file = ca_file;
}

void TransferBase::set_ssl_ca_buffer(const std::string& ca_buffer)
{
	_ca_buffer = ca_buffer;
}

void TransferBase::get_process(double& process_now, double& process_total)
{
	process_now = _process_now;
	process_total = _process_total;
}
void TransferBase::cancel()
{
	log::Debug() << "TransferBase::cancel";
	_ctl_run = CTL_CANCEL;
	_try_count = 0;
}
void TransferBase::reset()
{
	_ctl_run = CTL_RUN;
	_process_now = 0;
	_process_total = 0;
	_df_full = false;
	_try_count = 0;
}

//解析URL里的域名用IP替换域名，如果解析失败则保持不变
std::string TransferBase::get_resolved_url(const std::string& request_url)
{
	std::string resolved_url = request_url;
	std::string head;
	std::string domain;
	std::string ip_address;

	//提取domain
	head = "http://";
	std::size_t pos = resolved_url.find(head);
	if(std::string::npos != pos)
	{
		pos = resolved_url.find("/", head.length());
		if(std::string::npos != pos)
		{
			domain = resolved_url.substr(head.length(), pos-head.length());
		}
		else
		{
			domain = resolved_url.substr(head.length(), resolved_url.length());
		}
	}

	if(domain.empty())
	{
		head = "https://";
		pos = resolved_url.find(head);
		if(std::string::npos != pos)
		{
			pos = resolved_url.find("/", head.length());
			if(std::string::npos != pos)
			{
				domain = resolved_url.substr(head.length(), pos-head.length());
			}
			else
			{
				domain = resolved_url.substr(head.length(), resolved_url.length());
			}
		}
	}

	if(!domain.empty())
	{
		ip_address = gethostbyname(domain);
		if(!ip_address.empty())
		{
			resolved_url.replace(head.length(), domain.length(), ip_address);
		}
	}

	log::Debug() << "TransferBase::get_resolved_url resolved_url :[" << resolved_url << "]";
	return resolved_url;
}

#ifdef ANDROID
void TransferBase::set_jvm(JavaVM* jvm)
{
	_s_jvm = jvm;
}
#endif

//如果解析不到返回空string
std::string TransferBase::gethostbyname(const std::string& domain)
{
	std::string ip_address;
	const char* cstr_domain = domain.c_str();
#ifdef ANDROID
	JNIEnv *env = NULL;
	jclass java_inet_address_class = NULL;
	jmethodID getbyname_method_id = NULL;
	jstring jstring_domain=env->NewStringUTF(cstr_domain);
	jmethodID gethostaddress_method_id = NULL;
	jobject java_inet_address_object = NULL;
	bool is_attach_thread = false;

	do{
		if(_s_jvm->GetEnv((void**)&env, JNI_VERSION_1_4) != JNI_OK)
		{
			is_attach_thread = true;
		}

		if(is_attach_thread)
		{
			if(_s_jvm->AttachCurrentThread(&env, NULL) != JNI_OK)
			{
				is_attach_thread = false;
				break;
			}
		}

		//获取类InetAddress
		java_inet_address_class = env->FindClass("java/net/InetAddress");
		if(NULL == java_inet_address_class)
		{
			break;
		}

		//获取getByName method id
		getbyname_method_id = env->GetStaticMethodID(java_inet_address_class, "getByName","(Ljava/lang/String;)Ljava/net/InetAddress;");
		if(NULL == getbyname_method_id)
		{
			break;
		}

		//调用getByName返回InetAddress对象
		java_inet_address_object = (jobject)(env->CallStaticObjectMethod(java_inet_address_class, getbyname_method_id,jstring_domain));
		if(NULL == java_inet_address_object)
		{
			break;
		}

		//getHostAddress
		gethostaddress_method_id = env->GetMethodID(java_inet_address_class, "getHostAddress","()Ljava/lang/String;");
		if(NULL == gethostaddress_method_id)
		{
			break;
		}

		jstring jstring_ip = (jstring)(env->CallObjectMethod(java_inet_address_object, gethostaddress_method_id));
		
		if(NULL != jstring_ip)
		{
			char* cstr_ip = NULL;
			cstr_ip = (char*) env->GetStringUTFChars(jstring_ip, 0);
			ip_address = cstr_ip;
			
			env->ReleaseStringUTFChars(jstring_ip, cstr_ip);
			env->DeleteLocalRef(jstring_ip);
		}

	}while(0);


	env->DeleteLocalRef(jstring_domain);

	if(NULL != java_inet_address_class)
	{
		env->DeleteLocalRef(java_inet_address_class);
	}

	if(NULL != java_inet_address_object)
	{
		env->DeleteLocalRef(java_inet_address_object);
	}

	if(is_attach_thread)
	{
		if(_s_jvm->DetachCurrentThread() != JNI_OK)
		{
			log::Debug() << "TransferBase::gethostbyname DetachCurrentThread failed";
		}
	}
#endif

	log::Debug() << "TransferBase::gethostbyname ip_address :[" << ip_address << "]";
	return ip_address;
}


bool TransferBase::parse_batch_http_header(batch_info_t* batch_info)
{
    bool result = false;
    std::vector<std::string> split_line_vec;
	split(batch_info->batch_response_info[batch_info->current_seq].http_header_buf, "\r\n", split_line_vec);

    int i = 0;
    for(std::vector<std::string>::iterator iter = split_line_vec.begin();
    iter != split_line_vec.end(); iter++)
    {
		
        //http return code
        if(0 == i)
        {
            std::vector<std::string> split_vec;
            split(*iter, " ", split_vec);
            if(split_vec.size() >= 2)
            {
                batch_info->batch_response_info[batch_info->current_seq].http_code = atoi(split_vec[1].c_str());
                result = true;
            }
			i++;
        }
        else
        {
            std::vector<std::string> split_vec;
            split(*iter, ": ", split_vec);
            if(2 == split_vec.size())
            {
                batch_info->batch_response_info[batch_info->current_seq].http_headers[split_vec[0]] = split_vec[1];

                if("Set-Cookie" == split_vec[0])
                {
                    curl_easy_setopt(batch_info->curl_handle, CURLOPT_COOKIELIST, (*iter).c_str());
                }
                
                result = true;
            }
        }
		if(*iter == "") i=0;
        
    }

    if(!result)
    {
		batch_info->error_info->error_code = BATCH_PROTOCOL_PARSE_FAILED;
        batch_info->error_info->error_msg.append("[parse_batch_http_header failed]");
    }
   
    return result;
}

bool TransferBase::parse_batch_protocol_head(batch_info_t* batch_info)
{
    bool result = false;
    
    rapidjson::Document document;
    if (document.Parse<0>(const_cast<char*>(batch_info->protocol_head_buf.c_str())).HasParseError())
    {
		batch_info->error_info->error_code = BATCH_PROTOCOL_PARSE_FAILED;
        batch_info->error_info->error_msg.append("[parse_batch_head json wrong]");
        return result;
    }

    if(document.HasMember("version") && document.HasMember("seq") 
        && document.HasMember("hsize") && document.HasMember("bdsize") &&
        document.HasMember("result"))
    {
        int seq = atoi(document["seq"].GetString());
        int hsize = document["hsize"].GetInt();
        int bdsize = document["bdsize"].GetInt();
        int batch_result = document["result"].GetInt();

        if((seq < 0) || (seq > batch_info->batch_list_size))
        {
			batch_info->error_info->error_code = BATCH_PROTOCOL_PARSE_FAILED;
            batch_info->error_info->error_msg.append("[parse_batch_head seq wrong]");
            return result;
        }

        batch_info->batch_response_info[seq].http_header_total = hsize;
        batch_info->batch_response_info[seq].http_body_total = bdsize;
        batch_info->batch_response_info[seq].batch_result = batch_result;
		batch_info->current_seq = seq;

		batch_info->batch_response_info[seq].http_body_now = 0;
		batch_info->batch_response_info[seq].http_header_now = 0;

        result = true;

	}

    return result;
}

int TransferBase::handle_batch_recv(char* data, size_t data_len, batch_info_t* batch_info)
{
    if(0 == data_len)
    {
        return 0;
    }

    int written = 0; 
    
    switch(batch_info->batch_next)
    {
        case BATCH_RECV_PROTOCOL_HEAD:
            {
                char* head_end = std::find(data, data+data_len, '\n');
                if(data+data_len == head_end)
                {
                    batch_info->protocol_head_buf.append(data, data_len);
                    return data_len;
                }
                else
                {
                    int read_len = head_end - data;
                    batch_info->protocol_head_buf.append(data, read_len);
                    if(!parse_batch_protocol_head(batch_info))
                    {
                        return 0;
                    }

                    if((0 == batch_info->batch_response_info[batch_info->current_seq].http_header_total) &&
                        (0==batch_info->batch_response_info[batch_info->current_seq].http_body_total))
                    {
                        batch_info->batch_response_info[batch_info->current_seq].is_http_header_finished = true;
                        batch_info->batch_response_info[batch_info->current_seq].is_http_body_finished = true;

                        //parse http header
                        if(batch_info->batch_response_info[batch_info->current_seq].http_headers.empty())
                        {
                            if(!parse_batch_http_header(batch_info))
                            {
                                return 0;
                            }
                        }

                        //next new one of the batch
                        batch_info->current_seq = -1;
                        batch_info->protocol_head_buf = "";
                        batch_info->batch_next = BATCH_RECV_PROTOCOL_HEAD;
                    }
                    else if(0 != batch_info->batch_response_info[batch_info->current_seq].http_header_total)
                    {
                        batch_info->batch_next = BATCH_RECV_HTTP_HEAD;
                    }
                    else
                    {
                        batch_info->batch_next = BATCH_RECV_HTTP_BODY;
                    }

					//protocol have '\n' so add 1
                    return read_len + 1 + TransferBase::handle_batch_recv(data + (read_len + 1), data_len - (read_len + 1), batch_info);
                }
            }
            break;
        case BATCH_RECV_HTTP_HEAD:
            {
                int read_len = 0;
                int seq = batch_info->current_seq;
                int left_len = batch_info->batch_response_info[seq].http_header_total - batch_info->batch_response_info[seq].http_header_now;

                read_len = (left_len <= data_len)? left_len : data_len;
                batch_info->batch_response_info[seq].http_header_buf.append(data, read_len);
                batch_info->batch_response_info[seq].http_header_now = batch_info->batch_response_info[seq].http_header_now + read_len;

                if(0 == left_len)
                {
                    if(0 == batch_info->batch_response_info[batch_info->current_seq].http_body_total)
                    {
                        //package : only header
                        batch_info->current_seq = -1;
                        batch_info->protocol_head_buf = "";
                        batch_info->batch_next = BATCH_RECV_PROTOCOL_HEAD;
                    }
                    else
                    {
                        //package : header and body ; header finished
                        if(!parse_batch_http_header(batch_info))
                        {
                            return 0;
                        }
                        
                        batch_info->batch_next = BATCH_RECV_HTTP_BODY;
                    }
                }

                return read_len + TransferBase::handle_batch_recv(data + read_len, data_len - read_len, batch_info); 
            }
            break;
        case BATCH_RECV_HTTP_BODY:
            {
				if(batch_info->batch_response_info[batch_info->current_seq].http_headers.empty())
                {
                    if(!parse_batch_http_header(batch_info))
                    {
                        return 0;
                    }
                }

                int read_len = 0;
                int seq = batch_info->current_seq;
                int left_len = batch_info->batch_response_info[seq].http_body_total - batch_info->batch_response_info[seq].http_body_now;

                read_len = (left_len <= data_len)? left_len : data_len;
                if(batch_info->batch_response_info[seq].to_file_name.empty())
                {
                    batch_info->batch_response_info[seq].http_body_buf.append(data, read_len);
                }
                else
                {
                    if(HTTP_RET_CODE_SUCCESS != batch_info->batch_response_info[seq].http_code)
                    {
                        batch_info->batch_response_info[seq].http_body_buf.append(data, read_len);
                    }
                    else
                    {
                        if(NULL == batch_info->batch_response_info[seq].to_file_handle)
                        {
							if (0 == batch_info->batch_response_info[seq].error_info->error_code)
							{
								batch_info->batch_response_info[seq].to_file_handle = fopen(batch_info->batch_response_info[seq].to_file_name.c_str(), "wb");
								if (NULL == batch_info->batch_response_info[seq].to_file_handle) 
								{
									batch_info->batch_response_info[seq].error_info->error_code = DOWNLOAD_FILE_OPEN_FAILED;
									batch_info->batch_response_info[seq].error_info->error_msg.append("download file open failed");
								}
							}
                        }
						
						if (NULL != batch_info->batch_response_info[seq].to_file_handle)
						{
							int written = fwrite(data, 1, read_len, batch_info->batch_response_info[seq].to_file_handle);
							fflush(batch_info->batch_response_info[seq].to_file_handle);
							if(written != read_len)
							{
								batch_info->batch_response_info[seq].error_info->error_code = DF_FULL;
								batch_info->batch_response_info[seq].error_info->error_msg.append("df full");

								//set batch tunnel error
								batch_info->error_info->error_code = DF_FULL;
								batch_info->error_info->error_msg.append("df full");

								return 0;
							}
						}
						
						//文件打不开数据丢掉继续收下面数据
                    }
                }
                
                batch_info->batch_response_info[seq].http_body_now = batch_info->batch_response_info[seq].http_body_now + read_len;

                if(0 == left_len)
                {   
                    batch_info->current_seq = -1;
                    batch_info->protocol_head_buf = "";
                    //new package
                    batch_info->batch_next = BATCH_RECV_PROTOCOL_HEAD;
                }

                return read_len + TransferBase::handle_batch_recv(data + read_len, data_len - read_len, batch_info); 
            }
            break;
        default:
            break;
    }

    return written;
}

int TransferBase::batch_response_write_buffer(char *data, size_t size, size_t nmemb, batch_info_t* batch_info)
{
    int written = 0;

    CURL* curl_handle = batch_info->curl_handle;
    long http_retcode = 0;
    curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE , &http_retcode);
    
    if(HTTP_RET_CODE_SUCCESS == http_retcode)
	{
        written = TransferBase::handle_batch_recv(data, size * nmemb, batch_info);
    }
    else
    {
        batch_info->error_stream->append(static_cast<char*>(data), size * nmemb);
        written = size * nmemb;
    }
    
    return written;
}

int TransferBase::handle_batch_read(char *send_buf, size_t send_buf_len, batch_upload_info_t* batch_upload_info)
{
    if(0 == send_buf_len)
    {
        return 0;
    }

	//send all request buffer
	if (batch_upload_info->current_seq == batch_upload_info->batch_list_size)
	{
		return 0;
	}

    upload_info_t& upload_info = batch_upload_info->batch_upload_info[batch_upload_info->current_seq];
    
    int read_len = 0;

    switch(batch_upload_info->batch_send_next)
    {
        case BATCH_SEND_PROTOCOL_HEAD:
            {
                int left_len = upload_info.protocol_head_total - upload_info.protocol_head_now;
                read_len = (send_buf_len <= left_len)?send_buf_len : left_len;
                
                memcpy(send_buf, upload_info.protocol_head_buf.c_str() + upload_info.protocol_head_now, read_len);
                upload_info.protocol_head_now = upload_info.protocol_head_now + read_len;

                if(0 == left_len)
                {
                    batch_upload_info->batch_send_next = BATCH_SEND_HTTP_BODY;
                }

                return read_len + handle_batch_read(send_buf + read_len, send_buf_len - read_len, batch_upload_info);
            }
            break;
        case BATCH_SEND_HTTP_BODY:
            {
                int left_len = upload_info.http_body_total - upload_info.http_body_now;
                read_len = (send_buf_len <= left_len)?send_buf_len : left_len;
				if (NULL != upload_info.from_file_handle)
				{
					int read = fread(send_buf, 1, read_len, upload_info.from_file_handle);

				}
				else
				{
					memcpy(send_buf, upload_info.http_body_buf->c_str() + upload_info.http_body_now, read_len);
				}
                
                upload_info.http_body_now = upload_info.http_body_now + read_len;

                if(0 == left_len)
                {
					batch_upload_info->current_seq = batch_upload_info->current_seq + 1;
                    batch_upload_info->batch_send_next = BATCH_SEND_PROTOCOL_HEAD;
                }

                return read_len + handle_batch_read(send_buf + read_len, send_buf_len - read_len, batch_upload_info);
            }
            break;
        default:
            break;
    }
    
    return read_len;
}

int TransferBase::batch_read_buffer(char *send_buf, size_t size, size_t nitems, batch_upload_info_t* batch_upload_info)
{
	int len = handle_batch_read(send_buf, size*nitems, batch_upload_info);
    return len;
}

int TransferBase::response_write_buffer(char *data, size_t size, size_t nmemb, std::string *buffer)
{
	int written = 0;
    if(NULL != buffer)
    {
        buffer->append(data, size * nmemb);
        written = size * nmemb;     
    }

	return written;
}

int TransferBase::download_write_file(void *ptr, size_t size, size_t nmemb, void *params)
{
	down_write_param_t* down_write_para = static_cast<down_write_param_t*>(params);
	TransferBase* transfer = down_write_para->transfer;
	CURL* curl_handle = down_write_para->curl_handle;
	long http_retcode = 0;
	curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE , &http_retcode);
	int written = 0;

	if(HTTP_RET_CODE_SUCCESS == http_retcode)
	{
		if(down_write_para->to_stream)
		{
			FILE* stream = static_cast<FILE*>(down_write_para->to_stream);
			written = fwrite(ptr, size, nmemb, stream);
			if(size*nmemb != written)
			{
				transfer->_df_full = true;
			}
		}
	}
	else
	{
		if(down_write_para->error_stream)
		{
			down_write_para->error_stream->append(static_cast<char*>(ptr), size * nmemb);
			written = size * nmemb;     
		}
	}

	return written;
}


int TransferBase::header_handler(void *ptr, size_t size, size_t nmemb, void *userdata) 
{
	if(userdata)
	{
		std::map<std::string, std::string>& response_headers = *static_cast<std::map<std::string, std::string>* >(userdata);
		
		std::string head_line;
		head_line.append(static_cast<char*>(ptr), size * nmemb);
		std::vector< std::string > split_vec;
		split(head_line, ": ", split_vec);
		if(2 == split_vec.size())
		{
			std::string str_val = split_vec[1].c_str();
			std::size_t pos = str_val.find("\r\n");
			if(std::string::npos != pos)
			{
				str_val.replace(pos, strlen("\r\n"), "");
			}

			response_headers[split_vec[0]] = str_val;
		}
	}

	return size * nmemb;
}

int TransferBase::upload_process(void* clientp, double dltotal, double dlnow, double ultotal, double ulnow)
{
	int result = 0;
    if(NULL != clientp)
    {
        TransferBase *pclient = static_cast<TransferBase*>(clientp);
        pclient->_process_now = ulnow;
        pclient->_process_total = ultotal;
    }

    return result;    
}

int TransferBase::download_process(void* clientp, double dltotal, double dlnow, double ultotal, double ulnow)
{
	int result = 0;
    if(NULL != clientp)
    {
        TransferBase *pclient = static_cast<TransferBase*>(clientp);
        pclient->_process_now = dlnow;
        pclient->_process_total = dltotal;
    }

    return result;
}

int TransferBase::read_buffer(void *ptr, size_t size, size_t nitems, void *stream)
{
	read_buf_arg_t *rarg = (read_buf_arg_t *)stream;
	int len = rarg->len - rarg->pos;
	if (len > size * nitems)
	{
		len = size * nitems;
	}
	
	memcpy(ptr, rarg->buf + rarg->pos, len);
	rarg->pos += len;
	
	return len;
}

CURLcode TransferBase::sslctx_function(CURL * curl, void * sslctx, void * parm)
{
	X509_STORE * store;
	X509 * cert=NULL;
	BIO * bio;
	char *pem = (char*)parm;
	/* get a BIO */
 	bio=BIO_new_mem_buf(pem, -1);
 	/* use it to read the PEM formatted certificate from memory into an X509
 	* structure that SSL can use
 	*/
 	PEM_read_bio_X509(bio, &cert, 0, NULL);
 	if (cert == NULL)
 	  printf("PEM_read_bio_X509 failed...\n");
 
 	/* get a pointer to the X509 certificate store (which may be empty!) */
 	store=SSL_CTX_get_cert_store((SSL_CTX *)sslctx);
 
 	/* add our certificate to this store */
 	if (X509_STORE_add_cert(store, cert)==0)
 	  printf("error adding certificate\n");

	/*free*/
	BIO_free(bio);
	X509_free(cert);
 
 	/* all set to go */
 	return CURLE_OK ;
}

unsigned char TransferBase::to_hex(unsigned char x)
{
	return  x > 9 ? x + 55 : x + 48;
}

std::string TransferBase::url_encode(const std::string& str)
{
	std::string str_temp = "";  
    size_t length = str.length();  
    for (size_t i = 0; i < length; i++)  
    {  
        if (isalnum((unsigned char)str[i]) ||   
            (str[i] == '-') ||  
            (str[i] == '_') ||   
            (str[i] == '.') ||   
            (str[i] == '~'))  
            str_temp += str[i];  
        else if (str[i] == ' ')  
            str_temp += "+";  
        else  
        {  
            str_temp += '%';  
            str_temp += to_hex((unsigned char)str[i] >> 4);  
            str_temp += to_hex((unsigned char)str[i] % 16);  
        }  
    }  
    return str_temp;  
}


void TransferBase::split(std::string str,std::string pattern, std::vector<std::string>& result)
{
	std::string::size_type pos;
	str+=pattern;
	int size=str.size();

	for(int i=0; i<size; i++)
	{
		pos=str.find(pattern,i);
		if(pos<size)
		{
			std::string s=str.substr(i,pos-i);
			result.push_back(s);
			i=pos+pattern.size()-1;
		}
	}
}

void TransferBase::set_ssl(CURL* curl_handle)
{
	if(!_use_ssl)
	{
		return;
	}

	//set ssl version
	//The default action. This will attempt to figure out the remote SSL protocol version, i.e. either SSLv3 or TLSv1 (but not SSLv2, which became disabled by default with 7.18.1).
	curl_easy_setopt(curl_handle, CURLOPT_SSLVERSION,CURL_SSLVERSION_DEFAULT);

	if(_is_verify_ca)
	{
		curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, 1L);
		
		if(_is_verify_host)
		{
			curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYHOST, 2L);
		}
		else
		{
			curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYHOST, 0L);
		}

		if(!_ca_file.empty())
		{
			curl_easy_setopt(curl_handle, CURLOPT_CAINFO, _ca_file.c_str());
			return;
		}

		if(!_ca_buffer.empty())
		{
			curl_easy_setopt(curl_handle, CURLOPT_SSL_CTX_FUNCTION, sslctx_function);
			curl_easy_setopt(curl_handle, CURLOPT_SSL_CTX_DATA, _ca_buffer.c_str());
			return;
		}
	}
	else
	{
		curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYHOST, 0L);
	}
}

bool TransferBase::request(
	const std::string& request_url, request_type_t request_type, 
	const std::string& from, const std::string& to, const std::string&custom_request, 
	std::map<std::string, std::string>* response_headers, std::string& response, 
	error_info_t& error_info, batch_info_t* batch_info, batch_upload_info_t* batch_upload_info)
{
	log::Debug() << "TransferBase::request begin: "<< request_url;
	bool result = false;

	if((NULL == _curl_handle) || (NULL == _multi_handle) || (NULL == _curl_share._curl_share_handle))
	{
		error_info.error_code = INIT_TRANSFER_FAILED;
		error_info.error_msg.append("transfer init failed");
		return result;
	}

    if(CTL_CANCEL == _ctl_run)
    {
    	error_info.error_code = CANCELED;
		error_info.error_msg.append("have canceled");
        return result;
    }

    int still_running = 1;
	struct curl_httppost *formpost=NULL;
	struct curl_httppost *lastptr=NULL;
	FILE *from_file = NULL;
	FILE *to_file = NULL;
	struct curl_slist * headers = NULL;
	read_buf_arg_t read_buf_arg;
	long upload_file_size = 0;
	bool is_put_upload_file = ("PUT" == custom_request);
	long http_retcode = 0;
	down_write_param_t down_write_para;
	bool is_curl_error = false;
	int curl_msg_numleft = 0;
	char curl_error_msg[CURL_ERROR_SIZE];
	memset(curl_error_msg, 0, CURL_ERROR_SIZE);

	//set dns server
// 	std::string dns_server_list;
// 	get_dns_server_list(dns_server_list);
// 	curl_easy_setopt(_curl_handle, CURLOPT_DNS_SERVERS, dns_server_list.c_str());

	//set share
	curl_easy_setopt( _curl_handle, CURLOPT_SHARE, _curl_share._curl_share_handle);

	//not use signal
	curl_easy_setopt(_curl_handle, CURLOPT_NOSIGNAL, 1L);
	
	//set timeout
	curl_easy_setopt(_curl_handle, CURLOPT_CONNECTTIMEOUT, CURL_TIMEOUT_SECOND);
	curl_easy_setopt(_curl_handle, CURLOPT_LOW_SPEED_LIMIT, 1L);
	curl_easy_setopt(_curl_handle, CURLOPT_LOW_SPEED_TIME, CURL_TIMEOUT_SECOND);

	//set ssl
	set_ssl(_curl_handle);

	//set cookie
	if(!_cookie.empty())
	{
		curl_easy_setopt(_curl_handle, CURLOPT_COOKIE, _cookie.c_str());
	}
	
	//set error buffer
	curl_easy_setopt(_curl_handle, CURLOPT_ERRORBUFFER, curl_error_msg);

    if(BATCH_DOWNLOAD == request_type)
    {
		curl_easy_setopt(_curl_handle, CURLOPT_POSTFIELDS, batch_info->request_buf.c_str());
	    curl_easy_setopt(_curl_handle, CURLOPT_POSTFIELDSIZE, (long)batch_info->request_buf.length());
    }

    if(BATCH_UPLOAD == request_type)
    {
		curl_easy_setopt(_curl_handle, CURLOPT_POST, 1L);
		curl_easy_setopt(_curl_handle, CURLOPT_POSTFIELDSIZE, batch_upload_info->request_len);
        curl_easy_setopt(_curl_handle, CURLOPT_READFUNCTION, batch_read_buffer);
        curl_easy_setopt(_curl_handle, CURLOPT_READDATA, batch_upload_info);
    }
	
	if(UPLOAD_FROM_FILE == request_type)
	{
		//上传文件前检查文件是否存在
		from_file = fopen(from.c_str(), "r");
		if(NULL == from_file)
		{
		    error_info.error_code = UPLOAD_FILE_OPEN_FAILED;
		    error_info.error_msg.append("upload file open failed");
			return result;
		}

		//get file size
		if(is_put_upload_file)
		{
			fseek(from_file, 0L, SEEK_END);
	    	upload_file_size = ftell(from_file);
	    	fseek(from_file, 0, SEEK_SET);
		}
		
		//put file 不需要提交表单
		if(!is_put_upload_file)
		{
			fclose(from_file);
			
			curl_formadd(&formpost,
	            &lastptr,
	            CURLFORM_COPYNAME, "file",
	            CURLFORM_FILE, from.c_str(),
	            CURLFORM_END);
		}
	}

	if(UPLOAD_FROM_BUFFER == request_type)
	{
		read_buf_arg.buf = const_cast<char*>(from.c_str());
		read_buf_arg.len = from.length();
		read_buf_arg.pos = 0;

		//put buffer 不需要提交表单
		if(!is_put_upload_file)
		{
			curl_formadd(&formpost, 
				&lastptr, 
				CURLFORM_COPYNAME,"file",
				CURLFORM_BUFFER,"file",
				CURLFORM_BUFFERPTR,from.c_str(),
				CURLFORM_BUFFERLENGTH, from.length(),
				CURLFORM_END);
		}
	}

	if(DOWNLOAD_TO_FILE == request_type)
	{
		to_file = fopen(to.c_str(), "wb");
	    if (NULL == to_file) 
	    {
	        error_info.error_code = DOWNLOAD_FILE_OPEN_FAILED;
	        error_info.error_msg.append("download file open failed");
	        return result;
	    }
	}
	
    {
    	//set request header
    	for(headers_t::iterator iter = _headers.begin(); iter != _headers.end(); iter++)
    	{
    		std::string header = iter->first + ": " + iter->second;
			headers = curl_slist_append(headers, header.c_str());
    	}
    	
    	if(!custom_request.empty())
    	{
    		curl_easy_setopt(_curl_handle, CURLOPT_CUSTOMREQUEST, custom_request.c_str());

			if(MOVE == request_type)
			{
				std::string destination = "Destination: ";
				destination = destination + to;
				headers = curl_slist_append(headers, destination.c_str());
			}
			//获取response headers
			if(META == request_type)
			{
				curl_easy_setopt(_curl_handle, CURLOPT_HEADERDATA, response_headers);
				curl_easy_setopt(_curl_handle, CURLOPT_HEADERFUNCTION, header_handler);
			}
    	}

		if(NULL != headers)
		{
			curl_easy_setopt(_curl_handle, CURLOPT_HTTPHEADER, headers);
		}

		//put
		if(is_put_upload_file && (UPLOAD_FROM_BUFFER == request_type))
		{
			curl_easy_setopt(_curl_handle, CURLOPT_POSTFIELDS, read_buf_arg.buf);
			curl_easy_setopt(_curl_handle, CURLOPT_POSTFIELDSIZE, (long)read_buf_arg.len);
		}
		//put
		if(is_put_upload_file && (UPLOAD_FROM_FILE == request_type))
		{
			curl_easy_setopt(_curl_handle, CURLOPT_UPLOAD, 1L);
			curl_easy_setopt(_curl_handle, CURLOPT_INFILE, from_file);
			curl_easy_setopt(_curl_handle, CURLOPT_INFILESIZE, upload_file_size);
		}
		
    	//post set form
    	if(!is_put_upload_file && 
            ((UPLOAD_FROM_FILE == request_type) || (UPLOAD_FROM_BUFFER == request_type)))
		{
        	curl_easy_setopt(_curl_handle, CURLOPT_HTTPPOST, formpost);
		}

        //set upload process
		if((UPLOAD_FROM_FILE == request_type) || (UPLOAD_FROM_BUFFER == request_type))
		{
			curl_easy_setopt(_curl_handle, CURLOPT_NOPROGRESS, 0L);
	        curl_easy_setopt(_curl_handle, CURLOPT_PROGRESSDATA, this);
	        curl_easy_setopt(_curl_handle, CURLOPT_PROGRESSFUNCTION, TransferBase::upload_process);
		}

        //set download process
        if((DOWNLOAD_TO_FILE == request_type) || (DOWNLOAD_TO_BUFFER == request_type) || 
            (BATCH_DOWNLOAD == request_type) || (BATCH_UPLOAD == request_type))
        {
            curl_easy_setopt(_curl_handle, CURLOPT_NOPROGRESS, 0L);
	        curl_easy_setopt(_curl_handle, CURLOPT_PROGRESSDATA, this);
	        curl_easy_setopt(_curl_handle, CURLOPT_PROGRESSFUNCTION, TransferBase::download_process);
        }

        //download write to file
		if(DOWNLOAD_TO_FILE == request_type)
		{
			down_write_para.transfer = this;
			down_write_para.to_stream = to_file;
			down_write_para.error_stream = &response;
			down_write_para.curl_handle = _curl_handle;

			curl_easy_setopt(_curl_handle, CURLOPT_WRITEFUNCTION, download_write_file);
			curl_easy_setopt(_curl_handle,   CURLOPT_WRITEDATA, &down_write_para); 
		}
        else if((BATCH_DOWNLOAD == request_type) || (BATCH_UPLOAD == request_type))//batch download write to buffer
        {
            batch_info->curl_handle = _curl_handle;
			batch_info->error_stream = &response;
            
            curl_easy_setopt(_curl_handle, CURLOPT_WRITEFUNCTION, TransferBase::batch_response_write_buffer);
        	curl_easy_setopt(_curl_handle, CURLOPT_WRITEDATA, batch_info);
        }
		else//download write to buffer
		{
			curl_easy_setopt(_curl_handle, CURLOPT_WRITEFUNCTION, TransferBase::response_write_buffer);
        	curl_easy_setopt(_curl_handle, CURLOPT_WRITEDATA, &response);
		}
		
		curl_easy_setopt(_curl_handle, CURLOPT_ACCEPT_ENCODING, "gzip");
		
		curl_easy_setopt(_curl_handle, CURLOPT_FOLLOWLOCATION, 1L);
		curl_easy_setopt(_curl_handle, CURLOPT_AUTOREFERER, 1L);
        std::string resolved_url = request_url;//get_resolved_url(request_url);
        curl_easy_setopt(_curl_handle, CURLOPT_URL, resolved_url.c_str());
		
        curl_multi_add_handle(_multi_handle, _curl_handle);
        while(CURLM_CALL_MULTI_PERFORM == curl_multi_perform(_multi_handle, &still_running)){};
        do 
        {
            struct timeval timeout;
            int rc; 

            fd_set fdread;
            fd_set fdwrite;
            fd_set fdexcep;
            int maxfd = -1;
            long curl_timeo = -1;

            FD_ZERO(&fdread);
			FD_ZERO(&fdwrite);
			FD_ZERO(&fdexcep);

            //set a suitable timeout to play around with 
            timeout.tv_sec = 1;
            timeout.tv_usec = 0;

            curl_multi_timeout(_multi_handle, &curl_timeo);
            if(curl_timeo >= 0) 
            {
                timeout.tv_sec = curl_timeo / 1000;
                if(timeout.tv_sec > 1)
                {
                    timeout.tv_sec = 1;
                }
                else
                {
                    timeout.tv_usec = (curl_timeo % 1000) * 1000;
                }
            }

            // get file descriptors from the transfers 
            curl_multi_fdset(_multi_handle, &fdread, &fdwrite, &fdexcep, &maxfd);
           
            rc = select(maxfd+1, &fdread, &fdwrite, &fdexcep, &timeout);
            switch(rc) 
            {
                case -1:
                    // select error 
                    
                case 0:
                default:
                    // timeout or readable/writable sockets 
                    while(CURLM_CALL_MULTI_PERFORM == curl_multi_perform(_multi_handle, &still_running)){};
                    break;
            }
        } while(still_running && (CTL_RUN == _ctl_run));
        //get http response code
        if(0 == still_running)
        {
        	curl_easy_getinfo(_curl_handle, CURLINFO_RESPONSE_CODE , &http_retcode);
        }

		//get error code
		CURLMsg *msg = curl_multi_info_read(_multi_handle, &curl_msg_numleft);
		if(NULL != msg)
		{
			is_curl_error = (CURLE_OK == msg->data.result)? false : true;
		}

		//reset curl can reuse
		curl_multi_remove_handle(_multi_handle, _curl_handle);		
		curl_easy_reset(_curl_handle);

		//start the cookie engine
		curl_easy_setopt(_curl_handle, CURLOPT_COOKIEFILE, _cookie_path.c_str());
		curl_easy_setopt(_curl_handle, CURLOPT_COOKIEJAR, _cookie_path.c_str());
        
		if(!is_put_upload_file && 
            ((UPLOAD_FROM_FILE == request_type) || (UPLOAD_FROM_BUFFER == request_type)))
		{
			// then cleanup the formpost chain 
        	curl_formfree(formpost);
		}

		if(is_put_upload_file && (UPLOAD_FROM_FILE == request_type))
		{
			fclose(from_file);
		}

		if(DOWNLOAD_TO_FILE == request_type)
		{
			fclose(to_file);
		}

		if(NULL != headers)
		{
			curl_slist_free_all(headers);
			headers = NULL;
		}
    }

	if(CTL_CANCEL == _ctl_run)
    {
        error_info.error_code = CANCELED;
        error_info.error_msg.append("have canceled");
    }
	else if((DOWNLOAD_TO_FILE == request_type) && _df_full)
    {
        error_info.error_code = DF_FULL;
        error_info.error_msg.append("df full");
    }
	else if (is_curl_error)
	{
		//批量请求协议错误会导致curl错误, 没有错误信息则使用curl错误信息
		if(0 == error_info.error_code)
		{
			error_info.error_code = NET_ERROR;
			error_info.error_msg.append(std::string("net error : ") + std::string(curl_error_msg));
		}
	}
    else if(HTTP_RET_CODE_SUCCESS == http_retcode) 
    {
        result = true;
    }
    else
    {
        get_error_info(http_retcode, response, error_info);
    }

	//retry
	if((BATCH_DOWNLOAD != request_type) && (BATCH_UPLOAD != request_type) && 
		(NET_ERROR == error_info.error_code) && (_try_count < REQUEST_TRY_COUNT))
	{
		//delete download file or buffer
		if (DOWNLOAD_TO_FILE == request_type)
		{
			remove(to.c_str());
		}

		error_info.error_code = 0;
		response = "";
		_try_count++;
		result = request(request_url, request_type, from, to, custom_request, response_headers, response, error_info);
	}
	
	if((NET_ERROR != error_info.error_code) || 
		((NET_ERROR == error_info.error_code) && (_try_count = REQUEST_TRY_COUNT)))
	{
		_try_count = 0;
	}	

	log::Debug() << "TransferBase::request end";
    return result;
}


int TransferBase::upload_process_handle(request_info_t* request_info, double dltotal, double dlnow, double ultotal, double ulnow)
{
	int result = 0;
	if(NULL != request_info)
	{
		request_info->process_now = ulnow;
		request_info->process_total = ultotal;
	}

	return result;    
}

int TransferBase::download_process_handle(request_info_t* request_info, double dltotal, double dlnow, double ultotal, double ulnow)
{
	int result = 0;
	if(NULL != request_info)
	{
		request_info->process_now = dlnow;
		request_info->process_total = dltotal;
	}

	return result;
}

int TransferBase::download_write_file_handle(void *ptr, size_t size, size_t nmemb, request_info_t* request_info)
{
	CURL* curl_handle = request_info->curl_handle;
	long http_retcode = 0;
	curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE , &http_retcode);
	int written = 0;

	if(HTTP_RET_CODE_SUCCESS == http_retcode)
	{
		if(request_info->to_file)
		{
			FILE* stream = static_cast<FILE*>(request_info->to_file);
			written = fwrite(ptr, size, nmemb, stream);
			if(size*nmemb != written)
			{
				request_info->is_df_full = true;
			}
		}
	}
	else
	{
		request_info->request_para.response.append(static_cast<char*>(ptr), size * nmemb);
		written = size * nmemb;     
	}

	return written;
}

bool TransferBase::set_common_curl_opt(request_info_t& request_info)
{
	bool result = false;

	CURL* curl_handle = request_info.curl_handle;
	request_para_t& request_para(request_info.request_para);

	//set dns server
	std::string dns_server_list;
// 	get_dns_server_list(dns_server_list);
// 	curl_easy_setopt(curl_handle, CURLOPT_DNS_SERVERS, dns_server_list.c_str());

	//set share
	curl_easy_setopt(curl_handle, CURLOPT_SHARE, _curl_share._curl_share_handle);

	//not use signal
	curl_easy_setopt(curl_handle, CURLOPT_NOSIGNAL, 1L);

	//set timeout
	curl_easy_setopt(curl_handle, CURLOPT_CONNECTTIMEOUT, CURL_TIMEOUT_SECOND);
	curl_easy_setopt(curl_handle, CURLOPT_LOW_SPEED_LIMIT, 1L);
	curl_easy_setopt(curl_handle, CURLOPT_LOW_SPEED_TIME, CURL_TIMEOUT_SECOND);

	//set ssl
	set_ssl(curl_handle);

	//set cookie
	curl_easy_setopt(curl_handle, CURLOPT_COOKIEFILE, _cookie_path.c_str());
	curl_easy_setopt(curl_handle, CURLOPT_COOKIEJAR, _cookie_path.c_str());
	if(!_cookie.empty())
	{
		curl_easy_setopt(curl_handle, CURLOPT_COOKIE, _cookie.c_str());
	}
	
	//set error buffer
	curl_easy_setopt(curl_handle, CURLOPT_ERRORBUFFER, request_info.curl_error_msg);

	//gzip
	curl_easy_setopt(curl_handle, CURLOPT_ACCEPT_ENCODING, "gzip");

	//redirect
	curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl_handle, CURLOPT_AUTOREFERER, 1L);

	//set request url
    std::string resolved_url = request_info.request_para.request_url;//get_resolved_url(request_info.request_para.request_url);
	curl_easy_setopt(curl_handle, CURLOPT_URL, resolved_url.c_str());

	//set request header
	for(headers_t::iterator iter = _headers.begin(); iter != _headers.end(); iter++)
	{
		std::string header = iter->first + ": " + iter->second;
		request_info.headers = curl_slist_append(request_info.headers, header.c_str());
	}

	//set custom
	if(!request_para.custom_request.empty())
	{
		curl_easy_setopt(curl_handle, CURLOPT_CUSTOMREQUEST, request_para.custom_request.c_str());
	}

	if(NULL != request_info.headers)
	{
		curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, request_info.headers);
	}

	result = true;
	return result;
}

bool TransferBase::set_upload_from_file_curl_opt(request_info_t& request_info)
{
	bool result = false;

	CURL* curl_handle = request_info.curl_handle;
	request_para_t& request_para(request_info.request_para);

	//上传文件前检查文件是否存在
	request_info.from_file = fopen(request_para.from.c_str(), "r");
	if(NULL == request_info.from_file)
	{
		request_para.error_info->error_code = UPLOAD_FILE_OPEN_FAILED;
		request_para.error_info->error_msg.append("upload file open failed");
		return result;
	}

	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, TransferBase::response_write_buffer);
	curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &request_para.response);

	curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 0L);
	curl_easy_setopt(curl_handle, CURLOPT_PROGRESSDATA, &request_info);
	curl_easy_setopt(curl_handle, CURLOPT_PROGRESSFUNCTION, TransferBase::upload_process_handle);

	//post overwrite
	if (!request_info.is_put_upload)
	{
		fclose(request_info.from_file);
		request_info.from_file = NULL;

		curl_formadd(&request_info.formpost,
			&request_info.lastptr,
			CURLFORM_COPYNAME, "file",
			CURLFORM_FILE, request_para.from.c_str(),
			CURLFORM_END);

		curl_easy_setopt(curl_handle, CURLOPT_HTTPPOST, request_info.formpost);
	}
	else
	{
		//get file size
		fseek(request_info.from_file, 0L, SEEK_END);
		request_info.upload_file_size = ftell(request_info.from_file);
		fseek(request_info.from_file, 0, SEEK_SET);

		//
		curl_easy_setopt(curl_handle, CURLOPT_UPLOAD, 1L);
		curl_easy_setopt(curl_handle, CURLOPT_INFILE, request_info.from_file);
		curl_easy_setopt(curl_handle, CURLOPT_INFILESIZE, request_info.upload_file_size);
	}

	
	result = true;
	return result;
}

bool TransferBase::set_upload_from_buffer_curl_opt(request_info_t& request_info)
{
	bool result = false;

	CURL* curl_handle = request_info.curl_handle;
	request_para_t& request_para(request_info.request_para);
	
	request_info.read_buf_arg.buf = const_cast<char*>(request_para.from.c_str());
	request_info.read_buf_arg.len = request_para.from.length();
	request_info.read_buf_arg.pos = 0;

	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, TransferBase::response_write_buffer);
	curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &request_para.response);

	curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 0L);
	curl_easy_setopt(curl_handle, CURLOPT_PROGRESSDATA, &request_info);
	curl_easy_setopt(curl_handle, CURLOPT_PROGRESSFUNCTION, TransferBase::upload_process_handle);

	//put buffer 不需要提交表单
	if(!request_info.is_put_upload)
	{
		curl_formadd(&request_info.formpost, 
			&request_info.lastptr, 
			CURLFORM_COPYNAME,"file",
			CURLFORM_BUFFER,"file",
			CURLFORM_BUFFERPTR,request_para.from.c_str(),
			CURLFORM_BUFFERLENGTH, request_para.from.length(),
			CURLFORM_END);

		curl_easy_setopt(curl_handle, CURLOPT_HTTPPOST, request_info.formpost);
	}
	else
	{
		curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, request_info.read_buf_arg.buf);
		curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDSIZE, (long)request_info.read_buf_arg.len);
	}
	
	result = true;
	return result;
}

bool TransferBase::set_download_to_file_curl_opt(request_info_t& request_info)
{
	bool result = false;

	CURL* curl_handle = request_info.curl_handle;
	request_para_t& request_para(request_info.request_para);

	request_info.to_file = fopen(request_para.to.c_str(), "wb");
	if (NULL == request_info.to_file) 
	{
		request_para.error_info->error_code = DOWNLOAD_FILE_OPEN_FAILED;
		request_para.error_info->error_msg.append("download file open failed");
		return result;
	}

	curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 0L);
	curl_easy_setopt(curl_handle, CURLOPT_PROGRESSDATA, &request_info);
	curl_easy_setopt(curl_handle, CURLOPT_PROGRESSFUNCTION, TransferBase::download_process_handle);

	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, download_write_file_handle);
	curl_easy_setopt(curl_handle,   CURLOPT_WRITEDATA, &request_info); 

	result = true;
	return result;
}

bool TransferBase::set_download_to_buffer_curl_opt(request_info_t& request_info)
{
	bool result = false;

	CURL* curl_handle = request_info.curl_handle;
	request_para_t& request_para(request_info.request_para);

	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, TransferBase::response_write_buffer);
	curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &request_para.response);

	curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 0L);
	curl_easy_setopt(curl_handle, CURLOPT_PROGRESSDATA, &request_info);
	curl_easy_setopt(curl_handle, CURLOPT_PROGRESSFUNCTION, TransferBase::download_process_handle);

	result = true;
	return result;
}

bool TransferBase::set_move_curl_opt(request_info_t& request_info)
{
	bool result = false;

	CURL* curl_handle = request_info.curl_handle;
	request_para_t& request_para(request_info.request_para);

	std::string destination = "Destination: ";
	destination = destination + request_para.to;
	request_info.headers = curl_slist_append(request_info.headers, destination.c_str());

	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, TransferBase::response_write_buffer);
	curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &request_para.response);

	result = true;
	return result;
}

bool TransferBase::set_meta_curl_opt(request_info_t& request_info)
{
	bool result = false;

	CURL* curl_handle = request_info.curl_handle;
	request_para_t& request_para(request_info.request_para);

	curl_easy_setopt(curl_handle, CURLOPT_HEADERDATA, &(request_para.response_headers));
	curl_easy_setopt(curl_handle, CURLOPT_HEADERFUNCTION, header_handler);

	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, TransferBase::response_write_buffer);
	curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &request_para.response);

	result = true;
	return result;
}

bool TransferBase::set_batch_download_curl_opt(request_info_t& request_info)
{
	bool result = false;

	CURL* curl_handle = request_info.curl_handle;
	request_para_t& request_para(request_info.request_para);

	curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, request_para.batch_info->request_buf.c_str());
	curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDSIZE, (long)request_para.batch_info->request_buf.length());

	curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 0L);
	curl_easy_setopt(curl_handle, CURLOPT_PROGRESSDATA, &request_info);
	curl_easy_setopt(curl_handle, CURLOPT_PROGRESSFUNCTION, TransferBase::download_process_handle);

	request_para.batch_info->curl_handle = curl_handle;
	request_para.batch_info->error_stream = &request_para.response;
	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, TransferBase::batch_response_write_buffer);
	curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, request_para.batch_info);

	result = true;
	return result;
}

bool TransferBase::set_batch_upload_curl_opt(request_info_t& request_info)
{
	bool result = false;

	CURL* curl_handle = request_info.curl_handle;
	request_para_t& request_para(request_info.request_para);

	curl_easy_setopt(curl_handle, CURLOPT_POST, 1L);
	curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDSIZE, request_para.batch_upload_info->request_len);
	curl_easy_setopt(curl_handle, CURLOPT_READFUNCTION, batch_read_buffer);
	curl_easy_setopt(curl_handle, CURLOPT_READDATA, request_para.batch_upload_info);

	curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 0L);
	curl_easy_setopt(curl_handle, CURLOPT_PROGRESSDATA, &request_info);
	curl_easy_setopt(curl_handle, CURLOPT_PROGRESSFUNCTION, TransferBase::download_process_handle);

	request_para.batch_info->curl_handle = curl_handle;
	request_para.batch_info->error_stream = &request_para.response;
	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, TransferBase::batch_response_write_buffer);
	curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, request_para.batch_info);

	result = true;
	return result;
}

bool TransferBase::set_curl_opt(request_info_t& request_info)
{
	bool result = false;

	switch(request_info.request_para.request_type)
	{
	case UPLOAD_FROM_FILE:
		result = set_upload_from_file_curl_opt(request_info);
		break;
	case UPLOAD_FROM_BUFFER:
		result = set_upload_from_buffer_curl_opt(request_info);
		break;
	case DOWNLOAD_TO_FILE:
		result = set_download_to_file_curl_opt(request_info);
		break;
	case DOWNLOAD_TO_BUFFER:
		result = set_download_to_buffer_curl_opt(request_info);
		break;
	case MOVE:
		result = set_move_curl_opt(request_info);
		break;
	case META:
		result = set_meta_curl_opt(request_info);
		break;
	case BATCH_DOWNLOAD:
		result = set_batch_download_curl_opt(request_info);
		break;
	case BATCH_UPLOAD:
		result = set_batch_upload_curl_opt(request_info);
		break;
	default:
		break;
	}

	if (result)
	{
		result = set_common_curl_opt(request_info);
	}
	
	return result;
}

bool TransferBase::add_request(request_info_t& request_info)
{
	bool result = false;

	error_info_t& error_info = *(request_info.request_para.error_info);
	CURL* curl_handle = request_info.curl_handle;

	if((NULL == curl_handle) || (NULL == _multi_handle) || (NULL == _curl_share._curl_share_handle))
	{
		error_info.error_code = INIT_TRANSFER_FAILED;
		error_info.error_msg.append("transfer init failed");
		
		return result;
	}

	if(CTL_CANCEL == _ctl_run)
	{
		error_info.error_code = CANCELED;
		error_info.error_msg.append("have canceled");
		
		return result;
	}

	request_info.is_put_upload = ("PUT" == request_info.request_para.custom_request);

	result = set_curl_opt(request_info);

	if (result)
	{
		curl_multi_add_handle(_multi_handle, curl_handle);
		request_info.is_add_request_succes = result;
	}
	
	return result;
}

bool TransferBase::get_perform_request_result(ctl_t ctl_run_status, int still_running, request_info_t request_info_pool[CURL_HANDLE_POOL_SIZE])
{
	bool result = false;

	//cancel
	if (CTL_CANCEL == ctl_run_status)
	{
		for (int i = 0; i < CURL_HANDLE_POOL_SIZE; i++)
		{
			request_info_t& request_info = request_info_pool[i];
			request_para_t& request_para(request_info.request_para);
			CURL* curl_handle = request_info.curl_handle;

			//如果curl_handle为空则没有添加
			if (NULL == curl_handle)
			{
				break;
			}

			CURLMcode code = curl_multi_remove_handle(_multi_handle, curl_handle);
		}
	}

	//get curl error info
	CURLMsg *msg;
	int msgs_left;
	while ((CTL_RUN == ctl_run_status) && (msg = curl_multi_info_read(_multi_handle, &msgs_left)))
	{
		if (CURLMSG_DONE != msg->msg)
		{
			continue;
		}

		bool found = false;
		int idx = 0;
		for (; idx < CURL_HANDLE_POOL_SIZE; idx++)
		{
			request_info_t& request_info = request_info_pool[idx];
			if (!request_info_pool[idx].is_add_request_succes)
			{
				continue;
			}

			found = (msg->easy_handle == request_info.curl_handle);
			if (found)
			{
				break;
			}
		}

		if (found)
		{
			if(0 == still_running)
			{
				curl_easy_getinfo(msg->easy_handle, CURLINFO_RESPONSE_CODE , &(request_info_pool[idx].http_retcode));
			}

			request_info_pool[idx].is_curl_error = (CURLE_OK == msg->data.result)? false : true;

			curl_multi_remove_handle(_multi_handle, msg->easy_handle);		
		}

	}

	//reset curl free resource
	for (int i = 0; i < CURL_HANDLE_POOL_SIZE; i++)
	{
		request_info_t& request_info = request_info_pool[i];
		request_para_t& request_para(request_info.request_para);
		CURL* curl_handle = request_info.curl_handle;

		//如果curl_handle为空则没有添加
		if (NULL == curl_handle)
		{
			break;
		}

		curl_easy_reset(curl_handle);

		curl_easy_setopt(curl_handle, CURLOPT_COOKIEFILE, _cookie_path.c_str());
		curl_easy_setopt(curl_handle, CURLOPT_COOKIEJAR, _cookie_path.c_str());

		//没有添加成功或请求已经成功（重试）
		if (!request_info.is_add_request_succes || request_info.is_request_success)
		{
			continue;
		}

		if (NULL != request_info.formpost)
		{
			curl_formfree(request_info.formpost);
			request_info.formpost = NULL;
		}

		if(NULL != request_info.headers)
		{
			curl_slist_free_all(request_info.headers);
			request_info.headers = NULL;
		}

		if (NULL != request_info.from_file)
		{
			fclose(request_info.from_file);
			request_info.from_file = NULL;
		}

		if (NULL != request_info.to_file)
		{
			fclose(request_info.to_file);
			request_info.to_file = NULL;
		}

		//get request result
		if(CTL_CANCEL == ctl_run_status)
		{
			request_para.error_info->error_code = CANCELED;
			request_para.error_info->error_msg.append("have canceled");
		}
		else if(request_info.is_df_full)
		{
			request_para.error_info->error_code = DF_FULL;
			request_para.error_info->error_msg.append("df full");
		}
		else if (request_info.is_curl_error)
		{
			//批量请求协议错误会导致curl错误, 没有错误信息则使用curl错误信息
			if(0 == request_para.error_info->error_code)
			{
				request_para.error_info->error_code = NET_ERROR;
				request_para.error_info->error_msg.append(std::string("net error : ") + std::string(request_info.curl_error_msg));
			}
		}
		else if(HTTP_RET_CODE_SUCCESS == request_info.http_retcode) 
		{
			request_info.is_request_success = true;
		}
		else
		{
			get_error_info(request_info.http_retcode, request_para.response, *(request_para.error_info));
		}

	}

	result = true;
	return result;
}

bool TransferBase::retry_request(request_info_t request_info_pool[CURL_HANDLE_POOL_SIZE])
{
	bool result = false;

	bool is_need_retry = false;
	for (int i = 0; i < CURL_HANDLE_POOL_SIZE; i++)
	{
		request_info_t& request_info = request_info_pool[i];
		request_para_t& request_para(request_info.request_para);

		if (NULL == request_info.curl_handle)
		{
			break;
		}

		if (NET_ERROR != request_para.error_info->error_code)
		{
			continue;
		}

		if (request_info.retry_count < REQUEST_TRY_COUNT)
		{
			is_need_retry = true;

			//retry must clear down file or buffer
			if (DOWNLOAD_TO_FILE == request_para.request_type)
			{
				remove(request_para.to.c_str());
			}

			request_para.error_info->error_code = 0;
			request_para.response = "";

			request_info.reset();
			request_info.retry_count = request_info.retry_count + 1;

			add_request(request_info);
		}
		else
		{
			request_info.retry_count = 0;
		}
		
	}

	if (is_need_retry)
	{
		log::Debug() << "TransferBase::retry_request begin";
		perform_request(request_info_pool);
		log::Debug() << "TransferBase::retry_request end";
	}

	result = true;
	return result;
}

bool TransferBase::perform_request(request_info_t request_info_pool[CURL_HANDLE_POOL_SIZE])
{
	log::Debug() << "TransferBase::perform_request begin";
	bool result = false;

	int still_running = 1;

	while(CURLM_CALL_MULTI_PERFORM == curl_multi_perform(_multi_handle, &still_running)){};
	do 
	{
		struct timeval timeout;
		int rc; 

		fd_set fdread;
		fd_set fdwrite;
		fd_set fdexcep;
		int maxfd = -1;
		long curl_timeo = -1;

		FD_ZERO(&fdread);
		FD_ZERO(&fdwrite);
		FD_ZERO(&fdexcep);

		//set a suitable timeout to play around with 
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;

		curl_multi_timeout(_multi_handle, &curl_timeo);
		if(curl_timeo >= 0) 
		{
			timeout.tv_sec = curl_timeo / 1000;
			if(timeout.tv_sec > 1)
			{
				timeout.tv_sec = 1;
			}
			else
			{
				timeout.tv_usec = (curl_timeo % 1000) * 1000;
			}
		}

		// get file descriptors from the transfers 
		curl_multi_fdset(_multi_handle, &fdread, &fdwrite, &fdexcep, &maxfd);

		rc = select(maxfd+1, &fdread, &fdwrite, &fdexcep, &timeout);
		switch(rc) 
		{
		case -1:
			// select error 

		case 0:
		default:
			// timeout or readable/writable sockets 
			while(CURLM_CALL_MULTI_PERFORM == curl_multi_perform(_multi_handle, &still_running)){};
			break;
		}
	} while(still_running && (CTL_RUN == _ctl_run));

	get_perform_request_result(_ctl_run, still_running, request_info_pool);

	retry_request(request_info_pool);

	log::Debug() << "TransferBase::perform_request end";
	result = true;
	return result;
}


}//namespace ezsync


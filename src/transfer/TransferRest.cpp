/**
 * @file TransferRest.cpp
 * @author yangxugang
 * @brief 
 *     本文件主要定义了TransferRest
 **
*/

#include <signal.h>
#include <assert.h>
#include <sstream>

#include "TransferRest.h"

namespace ezsync{

TransferRest::TransferRest(const std::string& domain, const std::string& request_path,
    headers_t headers, const std::string& cookie_path, const std::string& batch_proxy_url, bool use_ssl):
	TransferBase(domain, "", headers, cookie_path, use_ssl), _request_path(request_path), _batch_proxy_url(batch_proxy_url)
{
    //去除路径后面的/
	std::size_t pos = _request_path.rfind("/");
	if((std::string::npos != pos) && (_request_path.length()-1) == pos)
	{
		_request_path.replace(pos, 1, "");
	}

	if(use_ssl)
	{
		_batch_proxy_url = std::string("https://") +  batch_proxy_url;
	}
	else
	{
		_batch_proxy_url = std::string("http://") +  batch_proxy_url;
	}	
		
}

TransferRest::~TransferRest()
{

}

bool TransferRest::batch_download_to_file(const std::vector<std::pair<std::string, std::string> >& request_list, std::vector<return_info_t>& return_list, error_info_t& error_info)
{
    bool result = false;

    batch_info_t batch_info;

	//init return list
	for (int i = 0; i < request_list.size(); i++)
	{
		return_info_t return_info;
		return_list.push_back(return_info);
	}
    
    std::ostringstream request_buf;
    int seq = 0;
    for(std::vector<std::pair<std::string, std::string> >::const_iterator iter = request_list.begin();
    iter != request_list.end(); iter++, seq++)
    {
        response_info_t response_info;
        response_info.to_file_name = iter->second;
        batch_info.batch_response_info[seq] = response_info;
        batch_info.batch_response_info[seq].error_info = &(return_list[seq].error_info);
        
        //gen one request head
        std::ostringstream header;
        std::string request_url;
        get_batch_url(iter->first, request_url);

        header << "GET " << request_url << " HTTP/1.1\r\n" 
            << "Accept: */*" << "\r\n"
            << "Host: " << _domain << "\r\n"
			<< "Accept-Encoding: deflate\r\n"
			<< "\r\n";

        request_buf << "{" 
            << "\"version\":" << "\"1\"," 
            << "\"seq\":" << "\"" << seq << "\","
            << "\"hsize\":" << "\"" << header.str().length() << "\","
            << "\"bdsize\":" << "\"" << 0 << "\""
            << "}"
            << "\r\n";

        request_buf << header.str();

    }

    batch_info.request_buf = request_buf.str();
    batch_info.batch_list_size = request_list.size();
	batch_info.error_info = &error_info;

    std::string response;
	result = request(_batch_proxy_url, BATCH_DOWNLOAD, "", "", "", NULL, response, error_info, &batch_info, NULL);

    int request_list_size = seq;
    for(seq = 0; seq < request_list_size; seq++)
    {
		//close file
		if(NULL != batch_info.batch_response_info[seq].to_file_handle)
		{
			fclose(batch_info.batch_response_info[seq].to_file_handle);
			batch_info.batch_response_info[seq].to_file_handle = NULL;
		}

		//http code 为 200 且收到完整的http header和完整的http body
        if((HTTP_RET_CODE_SUCCESS == batch_info.batch_response_info[seq].http_code) &&
			(batch_info.batch_response_info[seq].is_http_header_finished) && (batch_info.batch_response_info[seq].is_http_body_finished) &&
			(0 == return_list[seq].error_info.error_code))
        {
            return_list[seq].is_success = true;
        }
        else
        {
            return_list[seq].is_success = false;

			//remove download file
			remove(batch_info.batch_response_info[seq].to_file_name.c_str());
			
			//没有错误信息
			if (0 == return_list[seq].error_info.error_code)
			{
				//没有返回单个请求的http头或收到http头但是body没有收完整
				if ((0 == batch_info.batch_response_info[seq].http_code) || (HTTP_RET_CODE_SUCCESS == batch_info.batch_response_info[seq].http_code))
				{
					//batch result 为初始值则用批量通道的错误信息否则说明批量服务到目标服务出错
					if (BATCH_RESULT_INIT_VALUE == batch_info.batch_response_info[seq].batch_result)
					{
						return_list[seq].error_info.error_code = error_info.error_code;
						return_list[seq].error_info.error_msg.append("batch tunnel error ");
						return_list[seq].error_info.error_msg.append(error_info.error_msg);
					}
					else
					{
						return_list[seq].error_info.error_code = BATCH_SERVER_TO_TARGET_SERVER_FAILED;
						std::ostringstream error_msg;
						error_msg << "batch to target server failed batch_result [" << batch_info.batch_response_info[seq].batch_result << "]";
						return_list[seq].error_info.error_msg.append(error_msg.str());
					}
				}
				else
				{
					get_error_info(batch_info.batch_response_info[seq].http_code, batch_info.batch_response_info[seq].http_body_buf, return_list[seq].error_info);
				}
			}

        }
    }

    return result;
}

bool TransferRest::batch_download_to_buffer(std::vector<std::pair<std::string, std::string> >& request_list, std::vector<return_info_t>& return_list, error_info_t& error_info)
{
	bool result = false;

    batch_info_t batch_info;

	//init return list
	for (int i = 0; i < request_list.size(); i++)
	{
		return_info_t return_info;
		return_list.push_back(return_info);
	}
    
    std::ostringstream request_buf;
    int seq = 0;
    for(std::vector<std::pair<std::string, std::string> >::const_iterator iter = request_list.begin();
    iter != request_list.end(); iter++, seq++)
    {
        response_info_t response_info;
        batch_info.batch_response_info[seq] = response_info;
        batch_info.batch_response_info[seq].error_info = &(return_list[seq].error_info);
        
        //gen one request head
        std::ostringstream header;
        std::string request_url;
        get_batch_url(iter->first, request_url);

        header << "GET " << request_url << " HTTP/1.1\r\n" 
            << "Accept: */*" << "\r\n"
            << "Host: " << _domain << "\r\n"
			<< "Accept-Encoding: deflate\r\n"
			<< "\r\n";

        request_buf << "{" 
            << "\"version\":" << "\"1\"," 
            << "\"seq\":" << "\"" << seq << "\","
            << "\"hsize\":" << "\"" << header.str().length() << "\","
            << "\"bdsize\":" << "\"" << 0 << "\""
            << "}"
            << "\n";

        request_buf << header.str();

    }

    batch_info.request_buf = request_buf.str();
    batch_info.batch_list_size = request_list.size();
	batch_info.error_info = &error_info;

    std::string response;
	result = request(_batch_proxy_url, BATCH_DOWNLOAD, "", "", "", NULL, response, error_info, &batch_info, NULL);

    int request_list_size = seq;
    for(seq = 0; seq < request_list_size; seq++)
    {
		//http code 为 200 且收到完整的http header和完整的http body
		if((HTTP_RET_CODE_SUCCESS == batch_info.batch_response_info[seq].http_code) &&
			(batch_info.batch_response_info[seq].is_http_header_finished) && (batch_info.batch_response_info[seq].is_http_body_finished))
        {
            return_list[seq].is_success = true;
			request_list[seq].second = batch_info.batch_response_info[seq].http_body_buf;
        }
        else
        {
            return_list[seq].is_success = false;
            
			//没有错误信息
			if (0 == return_list[seq].error_info.error_code)
			{
				//没有返回单个请求的http头或收到http头但是body没有收完整
				if ((0 == batch_info.batch_response_info[seq].http_code) || (HTTP_RET_CODE_SUCCESS == batch_info.batch_response_info[seq].http_code))
				{
					//batch result 为初始值则用批量通道的错误信息否则说明批量服务到目标服务出错
					if (BATCH_RESULT_INIT_VALUE == batch_info.batch_response_info[seq].batch_result)
					{
						return_list[seq].error_info.error_code = error_info.error_code;
						return_list[seq].error_info.error_msg.append("batch tunnel error ");
						return_list[seq].error_info.error_msg.append(error_info.error_msg);
					}
					else
					{
						return_list[seq].error_info.error_code = BATCH_SERVER_TO_TARGET_SERVER_FAILED;
						std::ostringstream error_msg;
						error_msg << "batch to target server failed batch_result [" << batch_info.batch_response_info[seq].batch_result << "]";
						return_list[seq].error_info.error_msg.append(error_msg.str());
					}
				}
				else
				{
					get_error_info(batch_info.batch_response_info[seq].http_code, batch_info.batch_response_info[seq].http_body_buf, return_list[seq].error_info);
				}
			}

        }
    }

    return result;
}

//如果上传文件不存在需要判断批量数据通道的返回错误
bool TransferRest::batch_upload_from_file(const std::vector<std::pair<std::string, std::string> >& request_list, bool is_overwrite, std::vector<return_info_t>& return_list, error_info_t& error_info)
{
    bool result = false;

    batch_info_t batch_info;
    batch_upload_info_t batch_upload_info;

	//init return list
	for (int i = 0; i < request_list.size(); i++)
	{
		return_info_t return_info;
		return_list.push_back(return_info);
	}
    
    int seq = 0;
    for(std::vector<std::pair<std::string, std::string> >::const_iterator iter = request_list.begin();
    iter != request_list.end(); iter++, seq++)
    {
		std::ostringstream request_buf;

        //
        response_info_t response_info;
        batch_info.batch_response_info[seq] = response_info;
        batch_info.batch_response_info[seq].error_info = &(return_list[seq].error_info);

        //
        FILE* from_file_handle = fopen((iter->first).c_str(), "rb");
		if (NULL == from_file_handle)
		{
			return_list[seq].error_info.error_code = UPLOAD_FILE_OPEN_FAILED;
			return_list[seq].error_info.error_msg.append("upload file open failed");

			error_info.error_code = UPLOAD_FILE_OPEN_FAILED;
			error_info.error_msg.append("upload file open failed");

			for (int i = 0; i < seq; i++)
			{
				if (NULL != batch_upload_info.batch_upload_info[i].from_file_handle)
				{
					fclose(batch_upload_info.batch_upload_info[i].from_file_handle);
					batch_upload_info.batch_upload_info[i].from_file_handle = NULL;
				}
			}

			return result;
		}
		
        fseek(from_file_handle,0,SEEK_END);
        int file_len = ftell(from_file_handle);
        fseek(from_file_handle,0,SEEK_SET);
        
        //gen one request head
        std::ostringstream header;
        std::string request_url;
        get_batch_url(iter->second, request_url);
        std::string http_method;
        if(is_overwrite)
        {
            http_method = "PUT ";
        }
        else
        {
            http_method = "POST ";
        }
        header << http_method << request_url << " HTTP/1.1\r\n" 
            << "Accept: */*" << "\r\n"
            << "Host: " << _domain << "\r\n"
			<< "Accept-Encoding: deflate\r\n"
            << "Content-Length: "<<file_len<<"\r\n"
			<< "\r\n";

        request_buf << "{" 
            << "\"version\":" << "\"1\"," 
			<< "\"seq\":" << "\"" << seq << "\","
			<< "\"hsize\":" << "\"" << header.str().length() << "\","
            << "\"bdsize\":" << "\"" << file_len << "\""
            << "}"
            << "\r\n";
       
        request_buf << header.str();

        upload_info_t upload_info;
        upload_info.from_file_handle = from_file_handle;
        upload_info.protocol_head_buf = request_buf.str();
        upload_info.protocol_head_total = upload_info.protocol_head_buf.length();
        upload_info.http_body_total = file_len;
		batch_upload_info.request_len = batch_upload_info.request_len + upload_info.protocol_head_total + upload_info.http_body_total;

        batch_upload_info.batch_upload_info[seq] = upload_info;
    }

	batch_info.batch_list_size = request_list.size();
	batch_info.error_info = &error_info;

    batch_upload_info.batch_list_size = seq;
    
    std::string response;
	result = request(_batch_proxy_url, BATCH_UPLOAD, "", "", "", NULL, response, error_info, &batch_info, &batch_upload_info);

    int request_list_size = seq;
    for(seq = 0; seq < request_list_size; seq++)
    {
		//http code 为 200 且收到完整的http header和完整的http body
		if((HTTP_RET_CODE_SUCCESS == batch_info.batch_response_info[seq].http_code) &&
			(batch_info.batch_response_info[seq].is_http_header_finished) && (batch_info.batch_response_info[seq].is_http_body_finished))
        {
            return_list[seq].is_success = true;
        }
        else
        {
            return_list[seq].is_success = false;
            
			//没有错误信息
			if (0 == return_list[seq].error_info.error_code)
			{
				//没有返回单个请求的http头或收到http头但是body没有收完整
				if ((0 == batch_info.batch_response_info[seq].http_code) || (HTTP_RET_CODE_SUCCESS == batch_info.batch_response_info[seq].http_code))
				{
					//batch result 为初始值则用批量通道的错误信息否则说明批量服务到目标服务出错
					if (BATCH_RESULT_INIT_VALUE == batch_info.batch_response_info[seq].batch_result)
					{
						return_list[seq].error_info.error_code = error_info.error_code;
						return_list[seq].error_info.error_msg.append("batch tunnel error ");
						return_list[seq].error_info.error_msg.append(error_info.error_msg);
					}
					else
					{
						return_list[seq].error_info.error_code = BATCH_SERVER_TO_TARGET_SERVER_FAILED;
						std::ostringstream error_msg;
						error_msg << "batch to target server failed batch_result [" << batch_info.batch_response_info[seq].batch_result << "]";
						return_list[seq].error_info.error_msg.append(error_msg.str());
					}
				}
				else
				{
					get_error_info(batch_info.batch_response_info[seq].http_code, batch_info.batch_response_info[seq].http_body_buf, return_list[seq].error_info);
				}
			}

        }

		//close file
		if (NULL != batch_upload_info.batch_upload_info[seq].from_file_handle)
		{
			fclose(batch_upload_info.batch_upload_info[seq].from_file_handle);
			batch_upload_info.batch_upload_info[seq].from_file_handle = NULL;
		}
    }

    return result;
}

bool TransferRest::batch_upload_from_buffer(const std::vector<std::pair<std::string, std::string> >& request_list, bool is_overwrite, std::vector<return_info_t>& return_list, error_info_t& error_info)
{
	bool result = false;

    batch_info_t batch_info;
    batch_upload_info_t batch_upload_info;

	//init return list
	for (int i = 0; i < request_list.size(); i++)
	{
		return_info_t return_info;
		return_list.push_back(return_info);
	}
    
    int seq = 0;
    for(std::vector<std::pair<std::string, std::string> >::const_iterator iter = request_list.begin();
    iter != request_list.end(); iter++, seq++)
    {
		std::ostringstream request_buf;

        //
        response_info_t response_info;
        batch_info.batch_response_info[seq] = response_info;
        batch_info.batch_response_info[seq].error_info = &(return_list[seq].error_info);

        //gen one request head
        std::ostringstream header;
        std::string request_url;
        get_batch_url(iter->second, request_url);
        std::string http_method;
        if(is_overwrite)
        {
            http_method = "PUT ";
        }
        else
        {
            http_method = "POST ";
        }
        header << http_method << request_url << " HTTP/1.1\r\n" 
            << "Accept: */*" << "\r\n"
            << "Host: " << _domain << "\r\n"
			<< "Accept-Encoding: deflate\r\n"
			<< "Content-Length: " << iter->first.length() << "\r\n"
			<< "\r\n";

        request_buf << "{" 
            << "\"version\":" << "\"1\"," 
            << "\"seq\":" << "\"" << seq << "\","
            << "\"hsize\":" << "\"" << header.str().length() << "\","
            << "\"bdsize\":" << "\"" << iter->first.length() << "\""
            << "}"
            << "\r\n";

        request_buf << header.str();

        upload_info_t upload_info;
        upload_info.protocol_head_buf = request_buf.str();
		upload_info.http_body_buf = &(iter->first);
        upload_info.protocol_head_total = upload_info.protocol_head_buf.length();
        upload_info.http_body_total = iter->first.length();
		batch_upload_info.request_len = batch_upload_info.request_len + upload_info.protocol_head_total + upload_info.http_body_total;

        batch_upload_info.batch_upload_info[seq] = upload_info;
    }

	batch_info.batch_list_size = request_list.size();
	batch_info.error_info = &error_info;

    batch_upload_info.batch_list_size = seq;
    
    std::string response;
	result = request(_batch_proxy_url, BATCH_UPLOAD, "", "", "", NULL, response, error_info, &batch_info, &batch_upload_info);

    int request_list_size = seq;
    for(seq = 0; seq < request_list_size; seq++)
    {
		//http code 为 200 且收到完整的http header和完整的http body
		if((HTTP_RET_CODE_SUCCESS == batch_info.batch_response_info[seq].http_code) &&
			(batch_info.batch_response_info[seq].is_http_header_finished) && (batch_info.batch_response_info[seq].is_http_body_finished))
        {
            return_list[seq].is_success = true;
        }
        else
        {
            return_list[seq].is_success = false;
            
			//没有错误信息
			if (0 == return_list[seq].error_info.error_code)
			{
				//没有返回单个请求的http头或收到http头但是body没有收完整
				if ((0 == batch_info.batch_response_info[seq].http_code) || (HTTP_RET_CODE_SUCCESS == batch_info.batch_response_info[seq].http_code))
				{
					//batch result 为初始值则用批量通道的错误信息否则说明批量服务到目标服务出错
					if (BATCH_RESULT_INIT_VALUE == batch_info.batch_response_info[seq].batch_result)
					{
						return_list[seq].error_info.error_code = error_info.error_code;
						return_list[seq].error_info.error_msg.append("batch tunnel error ");
						return_list[seq].error_info.error_msg.append(error_info.error_msg);
					}
					else
					{
						return_list[seq].error_info.error_code = BATCH_SERVER_TO_TARGET_SERVER_FAILED;
						std::ostringstream error_msg;
						error_msg << "batch to target server failed batch_result [" << batch_info.batch_response_info[seq].batch_result << "]";
						return_list[seq].error_info.error_msg.append(error_msg.str());
					}
				}
				else
				{
					get_error_info(batch_info.batch_response_info[seq].http_code, batch_info.batch_response_info[seq].http_body_buf, return_list[seq].error_info);
				}
			}

        }

    }

    return result;
}

bool TransferRest::parse_error(const std::string& response, error_info_t& error_info)
{
    bool result = false;

    rapidjson::Document document;
    if (document.Parse<0>(const_cast<char*>(response.c_str())).HasParseError())
    {
        return result;
    }

	if(document.HasMember("Error") && document["Error"].IsObject())
    {
    	const rapidjson::Value& value = document["Error"];
		
        if(value.HasMember("code") && value["code"].IsString())
	    {
	        error_info.error_code = atoi(value["code"].GetString());
	    }
	    if(value.HasMember("Message") && value["Message"].IsString())
	    {
	        error_info.error_msg = value["Message"].GetString();
	    }
    }

    return result;
}

void TransferRest::get_error_info(long http_retcode, const std::string& response, error_info_t& error_info)
{
	error_info.error_msg.append(response);
    
    if(409 == http_retcode)
    {
        error_info.error_code = FILE_EXIST;
    }
    else if(404 == http_retcode)
    {
        error_info.error_code = FILE_NOT_EXIST;
    }
    else if(403 == http_retcode)
    {
        error_info.error_code = AUTH_FAILED;
    }
	else if((http_retcode > 400) && (http_retcode < 500))
	{
        error_info.error_code = SERVER_REQUEST_ERROR;
	}
    else
    {
        error_info.error_code = SERVER_INTERNAL_ERROR;
    }
}

void TransferRest::get_url(const std::string& path, std::string& request_url)
{
	if(_use_ssl)
    {
		request_url = "https";
    }
    else
    {
        request_url = "http";
    }

    std::string add_path;
    std::size_t pos = path.find("/");
	if((std::string::npos == pos) || (0 != pos))
	{
		add_path = "/";
	}
    
	request_url = request_url + "://" + _domain + _request_path + add_path + path;
}

void TransferRest::get_batch_url(const std::string& path, std::string& request_url)
{
    std::string add_path;
    std::size_t pos = path.find("/");
	if((std::string::npos == pos) || (0 != pos))
	{
		add_path = "/";
	}
    
	request_url = _request_path + add_path + path;
}
bool TransferRest::upload_from_file(const std::string& from, const std::string& to, bool is_overwrite, error_info_t& error_info)
{
	bool result = false;
	
	if(from.empty() || to.empty())
	{
		error_info.error_code = PARAM_EMPTY;
		error_info.error_msg = "param empty";
		return result;
	}
    
    std::string custom_request;
    if(is_overwrite)
    {
        custom_request = "PUT";
    }
	
	std::string request_url;
	get_url(to, request_url);
	
	std::string response;
	result = request(request_url, UPLOAD_FROM_FILE, from, to, custom_request, NULL, response, error_info);

	return result;
}

bool TransferRest::upload_from_buffer(const std::string& from, const std::string& to, bool is_overwrite, error_info_t& error_info)
{
	bool result = false;
	
	if(to.empty())
	{
		error_info.error_code = PARAM_EMPTY;
		error_info.error_msg = "param empty";
		return result;
	}

    std::string custom_request;
    if(is_overwrite)
    {
        custom_request = "PUT";
    }
	
	std::string request_url;
	get_url(to, request_url);
	
	std::string response;
	result = request(request_url, UPLOAD_FROM_BUFFER, from, to, custom_request, NULL, response, error_info);

	return result;
}

bool TransferRest::download_to_file(const std::string& from, const std::string& to, error_info_t& error_info)
{
	bool result = false;
	
	if(from.empty() || to.empty())
	{
		error_info.error_code = PARAM_EMPTY;
		error_info.error_msg = "param empty";
		return result;
	}
	
	std::string request_url;
	get_url(from, request_url);
	
	std::string response;
	result = request(request_url, DOWNLOAD_TO_FILE, from, to, "", NULL, response, error_info);
	if(!result)
	{
		remove(to.c_str());
	}

	return result;
}

bool TransferRest::download_to_buffer(const std::string& from, std::string& to, error_info_t& error_info)
{
	bool result = false;
	
	if(from.empty())
	{
		error_info.error_code = PARAM_EMPTY;
		error_info.error_msg = "param empty";
		return result;
	}
	
	std::string request_url;
	get_url(from, request_url);
	
	result = request(request_url, DOWNLOAD_TO_BUFFER, from, "", "", NULL, to, error_info);

	return result;
}

bool TransferRest::makedir(const std::string& dir, error_info_t& error_info)
{
	bool result = false;
	return result;
}

bool TransferRest::list(const std::string& dir, std::list<enum_item_t>& enum_list, error_info_t& error_info)
{
	bool result = false;
	
	if(dir.empty())
	{
		error_info.error_code = PARAM_EMPTY;
		error_info.error_msg = "param empty";
		return result;
	}
	
	std::string request_url;
    std::string dir_str;
    if("/" != dir)
    {
        dir_str = dir + "/";
    }
    else
    {
        dir_str = dir;
    }
    
	get_url(dir_str, request_url);
	
	std::string response;
	if(request(request_url, LIST, "", "", "", NULL, response, error_info))
	{
		rapidjson::Document document;
    	if (document.Parse<0>(const_cast<char*>(response.c_str())).HasParseError())
    	{
    		error_info.error_code = PARSE_RESPONSE_FAILED;
            error_info.error_msg = "parse response failed";
        	return result;
    	}

	    if(document.HasMember("object_list") && document["object_list"].IsArray())
	    {
	        const rapidjson::Value& array = document["object_list"];
			for(int i = 0; i < array.Size(); i++)
			{
				enum_item_t enum_item;

                if(array[i].HasMember("object") && array[i]["object"].IsString())
				{
					enum_item.path = array[i]["object"].GetString();

                    std::size_t pos = enum_item.path.find(dir_str);
					if((std::string::npos != pos) && (0 == pos))
					{
						enum_item.path.replace(pos, dir_str.length(), "");
					}
				}
	
				if(array[i].HasMember("mdatetime") && array[i]["mdatetime"].IsString())
				{
					enum_item.meta.mtime = atoi(array[i]["mdatetime"].GetString());
				}

				if(array[i].HasMember("content_md5") && array[i]["content_md5"].IsString())
				{
					enum_item.meta.md5 = array[i]["content_md5"].GetString();
				}

				if(array[i].HasMember("size") && array[i]["size"].IsString())
				{
					enum_item.meta.size = atoi(array[i]["size"].GetString());
				}

				if(array[i].HasMember("is_dir") && array[i]["is_dir"].IsString())
				{
					enum_item.meta.isdir = (1 == atoi(array[i]["is_dir"].GetString()))? true : false;
				}

				enum_list.push_back(enum_item);
			}

			result = true;
	    }
		else
		{
			error_info.error_code = PARSE_RESPONSE_FAILED;
            error_info.error_msg = "parse response failed";
        	return result;
		}
	}

	return result;
}

bool TransferRest::del(const std::string& path, error_info_t& error_info)
{
	bool result = false;
	
	if(path.empty())
	{
		error_info.error_code = PARAM_EMPTY;
		error_info.error_msg = "param empty";
		return result;
	}

	std::string request_url;
	get_url(path, request_url);
	
	std::string response;
	result = request(request_url, DEL, "", "", "DELETE", NULL, response, error_info);

	return result;
}

bool TransferRest::move(const std::string& from, const std::string& to, error_info_t& error_info)
{
	bool result = false;
	
	if(from.empty() || to.empty())
	{
		error_info.error_code = PARAM_EMPTY;
		error_info.error_msg = "param empty";
		return result;
	}

	std::string request_url;
	get_url(from, request_url);

    std::string add_path;
    std::size_t pos = to.find("/");
	if((std::string::npos == pos) || (0 != pos))
	{
		add_path = "/";
	}
	
	std::string response;
	result = request(request_url, MOVE, "", _request_path + add_path + to, "MOVE", NULL, response, error_info);

	return result;
}

bool TransferRest::meta(const std::string& path, meta_info_t& meta_info, error_info_t& error_info)
{
	bool result = false;
	
	if(path.empty())
	{
		error_info.error_code = PARAM_EMPTY;
		error_info.error_msg = "param empty";
		return result;
	}
	
	std::string request_url;
	get_url(path, request_url);

    std::map<std::string, std::string> response_headers;
	std::string response;
	if(request(request_url, META, "", "", "HEAD", &response_headers, response, error_info))
	{
		meta_info.isdir = false;

        std::map<std::string, std::string>::iterator iter;

        iter = response_headers.find("Content-MD5");
        if(response_headers.end() != iter)
        {
            meta_info.md5 = response_headers["Content-MD5"];
        }

        iter = response_headers.find("Content-Length");
        if(response_headers.end() != iter)
        {
            meta_info.size = atoi(response_headers["Content-Length"].c_str());
        }

        //Last-Modified: Fri, 21 Feb 2014 07:04:13 GMT
        iter = response_headers.find("Last-Modified");
        if(response_headers.end() != iter)
        {
            meta_info.size = atoi(response_headers["Last-Modified"].c_str());
        }

        meta_info.isdir = false;

        result = true;
	}

	return result;
}

}//namespace ezsync


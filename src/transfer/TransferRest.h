/**
 * @file TransferRest.h
 * @author yangxugang
 * @brief 
 *     本文件主要定义了TransferRest类
 **/
 #ifndef TRANSFER_REST_H
 #define TRANSFER_REST_H

 #include "TransferBase.h"

namespace ezsync{

class TransferRest : public TransferBase
{
public:
    TransferRest(const std::string& domain, const std::string& request_path, headers_t headers, const std::string& cookie_path, const std::string& batch_proxy_url, bool use_ssl);
    virtual ~TransferRest();

public:
	virtual bool makedir(const std::string& dir, error_info_t& error_info);
    virtual bool upload_from_file(const std::string& from, const std::string& to, bool is_overwrite, error_info_t& error_info);
	virtual bool upload_from_buffer(const std::string& from, const std::string& to, bool is_overwrite, error_info_t& error_info);
    virtual bool download_to_file(const std::string& from, const std::string& to, error_info_t& error_info);
	virtual bool download_to_buffer(const std::string& from, std::string& to, error_info_t& error_info);
    virtual bool list(const std::string& dir, std::list<enum_item_t>& enum_list, error_info_t& error_info);
	virtual bool del(const std::string& path, error_info_t& error_info);
	virtual bool move(const std::string& from, const std::string& to, error_info_t& error_info);
	virtual bool meta(const std::string& path, meta_info_t& meta_info, error_info_t& error_info);

	virtual bool batch_download_to_file(const std::vector<std::pair<std::string, std::string> >& request_list, std::vector<return_info_t>& return_list, error_info_t& error_info);
	virtual bool batch_download_to_buffer(std::vector<std::pair<std::string, std::string> >& request_list, std::vector<return_info_t>& return_list, error_info_t& error_info);
	virtual bool batch_upload_from_file(const std::vector<std::pair<std::string, std::string> >& request_list, bool is_overwrite, std::vector<return_info_t>& return_list, error_info_t& error_info);
	virtual bool batch_upload_from_buffer(const std::vector<std::pair<std::string, std::string> >& request_list, bool is_overwrite, std::vector<return_info_t>& return_list, error_info_t& error_info);

private:
	virtual void get_error_info(long http_retcode, const std::string& response, error_info_t& error_info);
	bool parse_error(const std::string& response, error_info_t& error_info);
	void get_url(const std::string& path, std::string& request_url);
	void get_batch_url(const std::string& path, std::string& request_url);
	
private:
	std::string _request_path;
	std::string _batch_proxy_url;
};

}//namespace ezsync
#endif //TRANSFER_CLIENT_H


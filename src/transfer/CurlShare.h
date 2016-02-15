/**
 * @file CurlShare.h
 * @author yangxugang
 * @brief 
 *     本文件主要定义了CurlShare类
 **/
#ifndef CURL_SHARE_H
#define CURL_SHARE_H
#include <pthread.h>
#include "curl/curl.h"

namespace ezsync{

class CurlShare
{
public:
	CurlShare():_curl_share_handle(NULL)
	{
#ifndef WIN32	
		if(0 != pthread_mutex_init(&_curl_share_mutex, NULL))
		{
			return;
		}
#else
		_curl_share_mutex = ::CreateMutex(NULL, FALSE, NULL); 
#endif

		_curl_share_handle = curl_share_init();

		if(NULL == _curl_share_handle)
		{
			_destory_mutex();
			return;
		}

		CURLSHcode scode = CURLSHE_OK;

		if ( CURLSHE_OK == scode ) 
		{
    		scode = curl_share_setopt( _curl_share_handle, CURLSHOPT_LOCKFUNC, CurlShare::_lock);
  		}
  		if ( CURLSHE_OK == scode ) 
		{
    		scode = curl_share_setopt( _curl_share_handle, CURLSHOPT_UNLOCKFUNC, CurlShare::_unlock);
  		}
		if ( CURLSHE_OK == scode ) 
		{
    		scode = curl_share_setopt( _curl_share_handle, CURLSHOPT_USERDATA, &_curl_share_mutex);
  		}
		if ( CURLSHE_OK == scode ) 
		{
    		scode = curl_share_setopt( _curl_share_handle, CURLSHOPT_SHARE, CURL_LOCK_DATA_COOKIE);
  		}
		if ( CURLSHE_OK == scode ) 
		{
    		scode = curl_share_setopt( _curl_share_handle, CURLSHOPT_SHARE, CURL_LOCK_DATA_DNS);
  		}
		if ( CURLSHE_OK == scode ) 
		{
    		scode = curl_share_setopt( _curl_share_handle, CURLSHOPT_SHARE, CURL_LOCK_DATA_SSL_SESSION);
  		}
		
		if(CURLSHE_OK != scode)
		{
			_destory_share();
			_destory_mutex();
			
			return;
		}
		
	}

	~CurlShare()
	{
		if(NULL != _curl_share_handle)
		{
			_destory_share();
			_destory_mutex();
		}
	}

	void _destory_mutex()
	{
#ifndef WIN32
		pthread_mutex_unlock(&_curl_share_mutex);
		pthread_mutex_destroy(&_curl_share_mutex);
#else
		::ReleaseMutex(_curl_share_mutex);
		::CloseHandle(_curl_share_mutex); 
#endif
	}

	void _destory_share()
	{
		curl_share_cleanup(_curl_share_handle);
		_curl_share_handle = NULL;
	}

public:
	static void _lock(CURL *handle, curl_lock_data data, curl_lock_access laccess, void *useptr)
	{
#ifndef WIN32	
		pthread_mutex_t* mutex = static_cast<pthread_mutex_t*>(useptr);
		pthread_mutex_lock(mutex);
#else
		HANDLE* mutex = static_cast<HANDLE*>(useptr);
		WaitForSingleObject(*mutex, INFINITE); 
#endif
	}

	static void _unlock(CURL *handle, curl_lock_data data, void *useptr)
	{
#ifndef WIN32
		pthread_mutex_t* mutex = static_cast<pthread_mutex_t*>(useptr);
		pthread_mutex_unlock(mutex);
#else
		HANDLE* mutex = static_cast<HANDLE*>(useptr);
		::ReleaseMutex(*mutex); 
#endif
	}

public:
	CURLSH *_curl_share_handle;
#ifndef WIN32	
	pthread_mutex_t _curl_share_mutex;
#else
	HANDLE _curl_share_mutex;
#endif
};

}//namespace ezsync

#endif//CURL_SHARE_H
#include "TransferRest.h"

using namespace ezsync;

void test_upload_from_file()
{
    headers_t headers;
	TransferInterface* transfer = new TransferRest("10.65.33.143:8081", "/file", headers, "", "", false);
    //std::string from = "/home/yangxugang01/work/src/transfer/eclipse-standard-kepler-SR1-win32-x86_64.zip";
    std::string from = "/home/yangxugang01/work/src/transfer/test.cpp";
    std::string to = "/2222.cpp";
   
    error_info_t error_info;

    if(transfer->upload_from_file(from, to, false, error_info))
    {
            printf("success\n");
    }
    else
    {
            printf("failed\n");
            printf("error_code [%d]\n", error_info.error_code);
            printf("error_msg [%s]\n", error_info.error_msg.c_str());
    }

    delete transfer;
}

void test_upload_from_buffer()
{
    headers_t headers;
	TransferInterface* transfer = new TransferRest("10.65.33.143:8081", "/file", headers, "", "", false);
    //std::string from = "/home/yangxugang01/work/src/transfer/eclipse-standard-kepler-SR1-win32-x86_64.zip";
    std::string from = "/home/yangxugang01/work/src/transfer/test.cpp";
    std::string to = "/2222.cpp";
   
    error_info_t error_info;

    if(transfer->upload_from_buffer(from, to, false, error_info))
    {
            printf("success\n");
    }
    else
    {
            printf("failed\n");
            printf("error_code [%d]\n", error_info.error_code);
            printf("error_msg [%s]\n", error_info.error_msg.c_str());
    }

    delete transfer;
}

void test_download_tofile()
{
    headers_t headers;
	TransferInterface* transfer = new TransferRest("10.65.33.143:8081", "/file", headers, "", "", false);
    //std::string from = "/test.cpp";
    std::string from = "/111111111111111-eclipse-standard-kepler-SR1-win32-x86_64.zip";
    std::string to = "./test_down.txt";

    error_info_t error_info;

    if(transfer->download_to_file(from, to, error_info))
    {
            printf("success\n");
    }
    else
    {
            printf("failed\n");
            printf("error_code [%d]\n", error_info.error_code);
            printf("error_msg [%s]\n", error_info.error_msg.c_str());
    }

    delete transfer;
}

void test_download_tobuffer()
{
    headers_t headers;
	TransferInterface* transfer = new TransferRest("10.65.33.143:8081", "/file", headers, "", "", false);
    std::string from = "/22222222222test.cpp";
    std::string to;
   
    error_info_t error_info;

    if(transfer->download_to_buffer(from, to, error_info))
    {
            printf("success\n");
			printf("down buffer is [%s]\n", to.c_str());
    }
    else
    {
            printf("failed\n");
            printf("error_code [%d]\n", error_info.error_code);
            printf("error_msg [%s]\n", error_info.error_msg.c_str());
    }

    delete transfer;
}

void test_makedir()
{
    headers_t headers;
	TransferInterface* transfer = new TransferRest("10.65.33.143:8081", "/file", headers, "", "", false);
    std::string dir = "/111test/123456";
   
    error_info_t error_info;

    if(transfer->makedir(dir, error_info))
    {
            printf("success\n");
    }
    else
    {
            printf("failed\n");
            printf("error_code [%d]\n", error_info.error_code);
            printf("error_msg [%s]\n", error_info.error_msg.c_str());
    }

    delete transfer;
}

void test_list()
{
    headers_t headers;
	TransferInterface* transfer = new TransferRest("10.65.33.143:8081", "/file", headers, "", "", false);
    std::string dir = "/";

    error_info_t error_info;
	std::list<enum_item_t> enum_list;

    if(transfer->list(dir, enum_list, error_info))
    {
            printf("success\n");
			for(std::list<enum_item_t>::iterator iter = enum_list.begin();
			iter != enum_list.end(); iter++)
			{
				printf("path is [%s]\n", iter->path.c_str());
                printf("md5 [%s] mtime [%d] size[%d] isdir [%d]\n",iter->meta.md5.c_str(), iter->meta.mtime, iter->meta.size, iter->meta.isdir);
			}
    }
    else
    {
            printf("failed\n");
            printf("error_code [%d]\n", error_info.error_code);
            printf("error_msg [%s]\n", error_info.error_msg.c_str());
    }

    delete transfer;
}

void test_del()
{
    headers_t headers;
	TransferInterface* transfer = new TransferRest("10.65.33.143:8081", "/file", headers, "", "", false);
    std::string path = "/test.cpp";

    error_info_t error_info;

    if(transfer->del(path, error_info))
    {
            printf("success\n");
    }
    else
    {
            printf("failed\n");
            printf("error_code [%d]\n", error_info.error_code);
            printf("error_msg [%s]\n", error_info.error_msg.c_str());
    }

    delete transfer;
}

void test_move()
{
	headers_t headers;
	TransferInterface* transfer = new TransferRest("10.65.33.143:8081", "/file", headers, "", "", false);
    std::string from = "/111test";
    std::string to = "/222test";

    error_info_t error_info;

    if(transfer->move(from, to, error_info))
    {
            printf("success\n");
    }
    else
    {
            printf("failed\n");
            printf("error_code [%d]\n", error_info.error_code);
            printf("error_msg [%s]\n", error_info.error_msg.c_str());
    }

    delete transfer;
}


void test_meta()
{
    headers_t headers;
	TransferInterface* transfer = new TransferRest("10.65.33.143:8081", "/file", headers, "", "", false);
    std::string path = "/make.sh";

    error_info_t error_info;
	meta_info_t meta;

    if(transfer->meta(path, meta, error_info))
    {
            printf("success\n");
			printf("md5 [%s] mtime [%d] size[%d] isdir [%d]\n",meta.md5.c_str(), meta.mtime, meta.size, meta.isdir);
    }
    else
    {
            printf("failed\n");
            printf("error_code [%d]\n", error_info.error_code);
            printf("error_msg [%s]\n", error_info.error_msg.c_str());
    }

    delete transfer;
}


int main(int argc, char* argv[])
{
    //test_upload();
    //test_download_tofile();
    //test_download_tobuffer();
    //test_makedir();
    //test_list();
    //test_del();
    //test_move();
    test_meta();
    return 0;
}

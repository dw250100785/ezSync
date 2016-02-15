#!/bin/bash
g++ -static -static-libstdc++ -static-libgcc -std=c++0x \
-L/usr/local/curl/lib/ \
-I./common/curl/include/ -I./common/openssl/include/ -I./ -I./common/boost/ -I./src/ -I./src/transfer/ -I./common/sqlite/ -I../../ -I./common/rapidjson/include/ \
./common/boost/libs/program_options/src/cmdline.cpp ./common/boost/libs/program_options/src/config_file.cpp ./common/boost/libs/program_options/src/convert.cpp \
./common/boost/libs/program_options/src/options_description.cpp ./common/boost/libs/program_options/src/parsers.cpp ./common/boost/libs/program_options/src/positional_options.cpp \
./common/boost/libs/program_options/src/split.cpp ./common/boost/libs/program_options/src/utf8_codecvt_facet.cpp ./common/boost/libs/program_options/src/value_semantic.cpp \
./common/boost/libs/program_options/src/variables_map.cpp ./common/boost/libs/program_options/src/winmain.cpp \
./common/boost/libs/filesystem/v2/src/v2_operations.cpp ./common/boost/libs/filesystem/v2/src/v2_path.cpp ./common/boost/libs/filesystem/v2/src/v2_portability.cpp \
./common/boost/libs/system/src/error_code.cpp ./src/md5.c ./src/utility.cpp ./src/file_storage.cpp ./src/sqlite_storage.cpp ./src/sqlite_wrap.cpp ./src/cloud_storage.cpp \
./src/cloud_version_history2.cpp ./src/transfer/TransferRest.cpp ./src/transfer/TransferBase.cpp ./src/file_sync_client_builder2.cpp ./src/sqlite_sync_client_builder2.cpp \
./src/path_clip_box.cpp ./src/ezsync.cpp -o ezsync_tool libcurl.a libcares.a libssl.a libsqlite3.a libz.a libcrypto.a libidn.a -lpthread -ldl -lrt 

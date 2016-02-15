LOCAL_PATH := $(call my-dir)
COMMON_PATH := $(LOCAL_PATH)/../../common

CFLAGS := -Os -Wpointer-arith -Wwrite-strings -Wunused -Winline \
 -Wnested-externs -Wmissing-declarations -Wmissing-prototypes -Wno-long-long \
 -Wfloat-equal -Wno-multichar -Wsign-compare -Wno-format-nonliteral \
 -Wendif-labels -Wstrict-prototypes -Wdeclaration-after-statement \
 -Wno-system-headers -DHAVE_CONFIG_H
 
#build c-ares static lib

include $(CLEAR_VARS)

include $(LOCAL_PATH)/../../common/c-ares/Makefile.inc


LOCAL_SRC_FILES := $(addprefix ../../common/c-ares/,$(CSOURCES))
LOCAL_CFLAGS += $(CFLAGS) -DANDROID
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../common/c-ares 

LOCAL_COPY_HEADERS_TO := libcares
LOCAL_COPY_HEADERS := $(addprefix ../../common/c-ares/,$(HHEADERS))

LOCAL_MODULE:= libcares

include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
include $(LOCAL_PATH)/../../common/curl/lib/Makefile.inc


LOCAL_SRC_FILES := $(addprefix ../../common/curl/lib/,$(CSOURCES))
LOCAL_CFLAGS += $(CFLAGS)
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../common/c-ares $(LOCAL_PATH)/../../common/curl/include/ $(LOCAL_PATH)/../../common/curl/lib $(LOCAL_PATH)/../../common/android-external-openssl-ndk-static-master/include $(LOCAL_PATH)/../../common/zlib 

LOCAL_MODULE:= libcurl

include $(BUILD_STATIC_LIBRARY)
#
include $(CLEAR_VARS)

LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../common/curl/include \
$(LOCAL_PATH)/../../common/curl/include \
$(LOCAL_PATH)/../../common/android-external-openssl-ndk-static-master/include \
$(LOCAL_PATH)/../../common/sqlite/ \
$(LOCAL_PATH)/../../common/rapidjson/include \
$(LOCAL_PATH)/../../common/boost_1_45_0 \
$(LOCAL_PATH)/../../ \
$(LOCAL_PATH)/../../common/

LOCAL_CFLAGS    += -DBOOST_FILESYSTEM_NO_LIB -Os
LOCAL_LDLIBS := -L$(SYSROOT)/usr/lib -llog -lsqlite -lz -lssl -lcrypto
LOCAL_STATIC_LIBRARIES := libcurl libcares
LOCAL_MODULE    := ezsync 
LOCAL_SRC_FILES := ../../common/boost_1_45_0/libs/filesystem/v2/src/v2_operations.cpp \
../../common/boost_1_45_0/libs/filesystem/v2/src/v2_path.cpp \
../../common/boost_1_45_0/libs/filesystem/v2/src/v2_portability.cpp \
../../common/boost_1_45_0/libs/system/src/error_code.cpp \
../../src/md5.c \
../../src/utility.cpp \
../../src/file_storage.cpp \
../../src/sqlite_storage.cpp \
../../src/sqlite_wrap.cpp \
../../src/cloud_storage.cpp \
../../src/cloud_version_history2.cpp \
../../src/transfer/TransferRest.cpp \
../../src/transfer/TransferBase.cpp \
../../src/file_sync_client_builder2.cpp \
../../src/sqlite_sync_client_builder2.cpp \
../../src/path_clip_box.cpp \
../../src/sqlite_transfer.cpp \
../../src/file_transfer.cpp \
ezsync.cpp \
jni_exception.cpp \
ezsync_client_wrap.cpp

include $(BUILD_SHARED_LIBRARY)

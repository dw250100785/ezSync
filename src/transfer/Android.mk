
LOCAL_PATH:= $(call my-dir)
COMMON_PATH := ../../common

CFLAGS := -Os -Wpointer-arith -Wwrite-strings -Wunused -Winline \
 -Wnested-externs -Wmissing-declarations -Wmissing-prototypes -Wno-long-long \
 -Wfloat-equal -Wno-multichar -Wsign-compare -Wno-format-nonliteral \
 -Wendif-labels -Wstrict-prototypes -Wdeclaration-after-statement \
 -Wno-system-headers -DHAVE_CONFIG_H

include $(CLEAR_VARS)
include $(COMMON_PATH)/curl/lib/Makefile.inc


LOCAL_SRC_FILES := $(addprefix $(COMMON_PATH)/curl/lib/,$(CSOURCES))
LOCAL_CFLAGS += $(CFLAGS)
LOCAL_C_INCLUDES += $(COMMON_PATH)/curl/include/ $(COMMON_PATH)/curl/lib $(COMMON_PATH)/openssl/include $(COMMON_PATH)/zlib 

LOCAL_MODULE:= libcurl

include $(BUILD_STATIC_LIBRARY)

#build c-ares static lib

include $(CLEAR_VARS)

include $(LOCAL_PATH)/c-ares/Makefile.inc


LOCAL_SRC_FILES := $(addprefix c-ares/,$(CSOURCES))
LOCAL_CFLAGS += $(CFLAGS) -DANDROID
LOCAL_C_INCLUDES += $(LOCAL_PATH)/c-ares 

LOCAL_COPY_HEADERS_TO := libcares
LOCAL_COPY_HEADERS := $(addprefix c-ares/,$(HHEADERS))

LOCAL_MODULE:= libcares

include $(BUILD_STATIC_LIBRARY)

#

# Build shared library now
# transfer 

include $(CLEAR_VARS)

LOCAL_MODULE := libtransfer
LOCAL_SRC_FILES := TransferClient.cpp
LOCAL_STATIC_LIBRARIES := libcurl
LOCAL_C_INCLUDES += $(COMMON_PATH)/curl/include/ $(COMMON_PATH)/rapidjson/include
LOCAL_LDLIBS    := -L./ -llog -lz -lssl -lcrypto
include $(BUILD_SHARED_LIBRARY) 


# Path information
LOCAL_PATH := $(call my-dir)
PROJECT_ROOT := $(LOCAL_PATH)
SOURCE_ROOT := $(PROJECT_ROOT)
INCLUDE_ROOT := $(PROJECT_ROOT)
THIRDPARTY_ROOT := $(PROJECT_ROOT)/..

# Source file information
COMMON_SOURCE_FILES := client.cpp
COMMON_SOURCE_FILES += server.cpp
COMMON_SOURCE_FILES += basic_tool.cpp
COMMON_SOURCE_FILES += config.cpp
COMMON_SOURCE_FILES += memreuse.cpp
COMMON_SOURCE_FILES += randomnumber.cpp
COMMON_SOURCE_FILES += crypto_tool.cpp
COMMON_SOURCE_FILES += mpz_tool.cpp
COMMON_SOURCE_FILES += jl10_psi_client.cpp
COMMON_SOURCE_FILES += jl10_psi_server.cpp
EXEC_SOURCE_FILES := main_adaptive.cpp

# Executable - For generating results from the SDDR Bloom filter vs. JL10
# PSI comparison figure in the paper.
include $(CLEAR_VARS)

LOCAL_MODULE := jl10psi_compare

LOCAL_SRC_FILES := $(COMMON_SOURCE_FILES)
LOCAL_SRC_FILES += $(EXEC_SOURCE_FILES)

LOCAL_CFLAGS += -ftree-vectorize
LOCAL_CPPFLAGS += -ftree-vectorize

LOCAL_C_INCLUDES := $(INCLUDE_ROOT)
LOCAL_C_INCLUDES += $(THIRDPARTY_ROOT)
LOCAL_C_INCLUDES += $(AOSP_ROOT)/external/openssl/include
LOCAL_C_INCLUDES += $(AOSP_ROOT)/system/core/include

LOCAL_SHARED_LIBRARIES += crypto c cutils dl
LOCAL_STATIC_LIBRARIES += gmp

include $(BUILD_EXECUTABLE)

# Static Library - Used by the EbNRadioBT2PSI radio to perform device
# recognition for selective linkability using PSI over Bluetooth 2.1
# connections
include $(CLEAR_VARS)

LOCAL_MODULE := jl10psi

LOCAL_SRC_FILES := $(COMMON_SOURCE_FILES)

LOCAL_CFLAGS += -ftree-vectorize
LOCAL_CPPFLAGS += -ftree-vectorize

LOCAL_C_INCLUDES := $(INCLUDE_ROOT)
LOCAL_C_INCLUDES += $(THIRDPARTY_ROOT)
LOCAL_C_INCLUDES += $(AOSP_ROOT)/external/openssl/include
LOCAL_C_INCLUDES += $(AOSP_ROOT)/system/core/include

include $(BUILD_STATIC_LIBRARY)


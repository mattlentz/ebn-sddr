# Path information
LOCAL_PATH := $(call my-dir)/..
PROJECT_ROOT := $(LOCAL_PATH)
SOURCE_ROOT := $(PROJECT_ROOT)/source
INCLUDE_ROOT := $(PROJECT_ROOT)/include
THIRDPARTY_ROOT := $(PROJECT_ROOT)/third-party

# Pre-built shared libraries that are required from the AOSP target build,
# and are not part of the NDK
include $(CLEAR_VARS)
LOCAL_MODULE := c
LOCAL_SRC_FILES := $(AOSP_ROOT)/out/target/product/$(PHONEMODEL)/obj/lib/libc.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := cutils
LOCAL_SRC_FILES := $(AOSP_ROOT)/out/target/product/$(PHONEMODEL)/obj/lib/libcutils.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := dl
LOCAL_SRC_FILES := $(AOSP_ROOT)/out/target/product/$(PHONEMODEL)/obj/lib/libdl.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := bluetooth
LOCAL_SRC_FILES := $(AOSP_ROOT)/out/target/product/$(PHONEMODEL)/obj/lib/libbluetooth.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := crypto
LOCAL_SRC_FILES := $(AOSP_ROOT)/out/target/product/$(PHONEMODEL)/obj/lib/libcrypto.so
include $(PREBUILT_SHARED_LIBRARY)

# Executable - Main SDDR application
include $(CLEAR_VARS)

LOCAL_MODULE := sddr

LOCAL_CFLAGS += -D$(PHONEMODEL_CAPS) -DLOG_W_ENABLED -DLOG_D_ENABLED -DLOG_P_ENABLED

LOCAL_SRC_FILES := $(SOURCE_ROOT)/Address.cpp
LOCAL_SRC_FILES += $(SOURCE_ROOT)/AndroidBluetooth.cpp
LOCAL_SRC_FILES += $(SOURCE_ROOT)/AndroidWake.cpp
LOCAL_SRC_FILES += $(SOURCE_ROOT)/Base64.cpp
LOCAL_SRC_FILES += $(SOURCE_ROOT)/BinaryToUTF8.cpp
LOCAL_SRC_FILES += $(SOURCE_ROOT)/BitMap.cpp
LOCAL_SRC_FILES += $(SOURCE_ROOT)/BloomFilter.cpp
LOCAL_SRC_FILES += $(SOURCE_ROOT)/BluetoothHCI.cpp
LOCAL_SRC_FILES += $(SOURCE_ROOT)/Config.cpp
LOCAL_SRC_FILES += $(SOURCE_ROOT)/EbNController.cpp
LOCAL_SRC_FILES += $(SOURCE_ROOT)/EbNDevice.cpp
LOCAL_SRC_FILES += $(SOURCE_ROOT)/EbNDeviceBT2.cpp
LOCAL_SRC_FILES += $(SOURCE_ROOT)/EbNDeviceBT4.cpp
LOCAL_SRC_FILES += $(SOURCE_ROOT)/EbNDeviceBT4AR.cpp
LOCAL_SRC_FILES += $(SOURCE_ROOT)/EbNHystPolicy.cpp
LOCAL_SRC_FILES += $(SOURCE_ROOT)/EbNRadio.cpp
LOCAL_SRC_FILES += $(SOURCE_ROOT)/EbNRadioBT2.cpp
LOCAL_SRC_FILES += $(SOURCE_ROOT)/EbNRadioBT2NR.cpp
LOCAL_SRC_FILES += $(SOURCE_ROOT)/EbNRadioBT2PSI.cpp
LOCAL_SRC_FILES += $(SOURCE_ROOT)/EbNRadioBT4.cpp
LOCAL_SRC_FILES += $(SOURCE_ROOT)/EbNRadioBT4AR.cpp
LOCAL_SRC_FILES += $(SOURCE_ROOT)/ECDH.cpp
LOCAL_SRC_FILES += $(SOURCE_ROOT)/Logger.cpp
LOCAL_SRC_FILES += $(SOURCE_ROOT)/RSErasureDecoder.cpp
LOCAL_SRC_FILES += $(SOURCE_ROOT)/RSErasureEncoder.cpp
LOCAL_SRC_FILES += $(SOURCE_ROOT)/RSMatrix.cpp
LOCAL_SRC_FILES += $(SOURCE_ROOT)/SegmentedBloomFilter.cpp
LOCAL_SRC_FILES += $(SOURCE_ROOT)/SharedArray.cpp
LOCAL_SRC_FILES += $(SOURCE_ROOT)/SipHash.cpp
LOCAL_SRC_FILES += $(SOURCE_ROOT)/Main.cpp
LOCAL_SRC_FILES += $(SOURCE_ROOT)/ebncore.pb.cc

LOCAL_C_INCLUDES += $(INCLUDE_ROOT)
LOCAL_C_INCLUDES += $(THIRDPARTY_ROOT)
LOCAL_C_INCLUDES += $(THIRDPARTY_ROOT)/google/src
LOCAL_C_INCLUDES += $(AOSP_ROOT)/external/openssl/include
LOCAL_C_INCLUDES += $(AOSP_ROOT)/system/core/include

LOCAL_LDLIBS += -llog

LOCAL_SHARED_LIBRARIES += bluetooth crypto c cutils dl
LOCAL_STATIC_LIBRARIES += jl10psi gmp protobuf jerasure csiphash

include $(BUILD_EXECUTABLE)

# Building necessary third-party modules
include $(THIRDPARTY_ROOT)/Android.mk


# Path information
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := protobuf
LOCAL_SRC_FILES := src/.libs/libprotobuf.a
include $(PREBUILT_STATIC_LIBRARY)


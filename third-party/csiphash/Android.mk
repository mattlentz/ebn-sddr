# Path information
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := csiphash
LOCAL_SRC_FILES := csiphash.c 
include $(BUILD_STATIC_LIBRARY)


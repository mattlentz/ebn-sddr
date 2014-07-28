# Path information
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := gmp
LOCAL_SRC_FILES := .libs/libgmp.a
include $(PREBUILT_STATIC_LIBRARY)


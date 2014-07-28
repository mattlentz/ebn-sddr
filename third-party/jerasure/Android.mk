# Path information
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := jerasure
LOCAL_SRC_FILES := galois.c jerasure.c reed_sol.c cauchy.c liberation.c 
include $(BUILD_STATIC_LIBRARY)


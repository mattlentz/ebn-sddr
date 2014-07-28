APP_ABI := armeabi-v7a
APP_STL := gnustl_shared
APP_CFLAGS += -O3
APP_CPPFLAGS += -std=gnu++11 -fexceptions -O3 -Wno-literal-suffix
APP_GNUSTL_FORCE_CPP_FEATURES := exceptions rtti

NDK_TOOLCHAIN_VERSION = 4.8

# Specifying the exact phone model and corresponding AOSP framework location
PHONEMODEL := maguro
PHONEMODEL_CAPS := MAGURO
AOSP_ROOT := /home/matt/Projects/android/system-4.1.2


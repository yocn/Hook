LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_LDLIBS    := -llog -ljnigraphics
LOCAL_MODULE    := venus
APP_PROJECT_PATH:= LOCAL_PATH
LOCAL_SRC_FILES := venus.cpp
LOCAL_CFLAGS    =  -ffast-math -O3 -funroll-loops
LOCAL_C_INCLUDES := $(LOCAL_PATH)/rapidjson
 
include $(BUILD_SHARED_LIBRARY)

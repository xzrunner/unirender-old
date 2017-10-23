INNER_SAVED_LOCAL_PATH := $(LOCAL_PATH)

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := unirender

LOCAL_CFLAGS += -std=gnu99

LOCAL_C_INCLUDES  := \
	${CLIB_PATH} \
	${UNIRENDER_SRC_PATH}/include \
	${UNIRENDER_SRC_PATH}/include/unirender \
	${UNIRENDER_SRC_PATH}/external \
	${LOGGER_SRC_PATH} \
	${MEMMGR_SRC_PATH}/include \

LOCAL_SRC_FILES := \
	$(subst $(LOCAL_PATH)/,,$(shell find $(LOCAL_PATH) -name "*.cpp" -print)) \
	$(subst $(LOCAL_PATH)/,,$(shell find $(LOCAL_PATH) -name "*.c" -print)) \

LOCAL_STATIC_LIBRARIES := \
	logger \

include $(BUILD_STATIC_LIBRARY)	

LOCAL_PATH := $(INNER_SAVED_LOCAL_PATH)
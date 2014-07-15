
LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE_PATH := $(DBG_OUT_PATH)
LOCAL_SRC_FILES:= \/par    uartTest.c
LOCAL_MODULE_TAGS:= optional
LOCAL_MODULE := uart_test
include $(BUILD_EXECUTABLE)

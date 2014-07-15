LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

#exp:
#	$(shell mkdir -p $(TARGET_OUT)/qrd_theme)
#	$(shell cp -r $(LOCAL_PATH)/PreLoadTheme_res/*.zip  $(TARGET_OUT)/qrd_theme/)

#PRODUCT_COPY_FILES += \
#	$(LOCAL_PATH)/system/bin/chkntfs:/system/bin/chkntfs \
#	$(LOCAL_PATH)/system/bin/mkntfs:/system/bin/mkntfs \
#	$(LOCAL_PATH)/system/bin/ntfs-3g:/system/bin/ntfs-3g \
	
################################
# /system/bin

include $(CLEAR_VARS)
LOCAL_MODULE := ntfs-3g
LOCAL_SRC_FILES := /system/bin/ntfs-3g
LOCAL_MODULE_CLASS := bin
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_OUT)/bin
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := chkntfs
LOCAL_SRC_FILES := /system/bin/chkntfs
LOCAL_MODULE_CLASS := bin
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_OUT)/bin
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := mkntfs
LOCAL_SRC_FILES := /system/bin/mkntfs
LOCAL_MODULE_CLASS := bin
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_OUT)/bin
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := fsck.exfat
LOCAL_SRC_FILES := /system/bin/fsck.exfat
LOCAL_MODULE_CLASS := bin
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_OUT)/bin
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := mkfs.exfat
LOCAL_SRC_FILES := /system/bin/mkfs.exfat
LOCAL_MODULE_CLASS := bin
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_OUT)/bin
include $(BUILD_PREBUILT)
# /system/bin
################################



################################
# /system/lib


# /system/lib
################################





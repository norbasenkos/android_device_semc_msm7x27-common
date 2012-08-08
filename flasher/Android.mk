# Author: nobodyAtall
# Flasher is a utility that installs a yaffs2 img file to an mtd partition
# If works at a filesystem level, not a block one (e.g. like flash_image)
# Detect partition from /proc/mtd -> Format -> Mount -> Extract yaffs2 image -> Umount
# CAUTION: Use this on an unmounted partition and preferably via recovery

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := flash_image.c roots.c mounts.c
LOCAL_MODULE := flasher
LOCAL_FORCE_STATIC_EXECUTABLE := true
LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)
LOCAL_MODULE_TAGS := optional
LOCAL_STATIC_LIBRARIES := libflashutils libmtdutils libmmcutils libbmlutils libunyaffs libcrecovery libcutils libc
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := flash_image.c roots.c mounts.c
LOCAL_MODULE := static_flasher
LOCAL_MODULE_STEM := flasher
LOCAL_FORCE_STATIC_EXECUTABLE := true
LOCAL_MODULE_PATH += $(TARGET_RECOVERY_ROOT_OUT)/sbin
LOCAL_MODULE_TAGS := optional
LOCAL_STATIC_LIBRARIES := libflashutils libmtdutils libmmcutils libbmlutils libunyaffs libcrecovery libcutils libc
include $(BUILD_EXECUTABLE)


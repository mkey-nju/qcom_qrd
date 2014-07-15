
ifeq ($(CONFIG_HAL_GPSLIB),y)
SUBDIR_MAKEFILES += $(call all-named-subdir-makefiles,gpslib)
endif

ifeq ($(CONFIG_HAL_SERVER),y)
SUBDIR_MAKEFILES += $(call all-named-subdir-makefiles,lidbg_servicer)
endif

ifeq ($(CONFIG_HAL_VOLD_8x25Q),y)
SUBDIR_MAKEFILES += $(call all-named-subdir-makefiles,8x25q_vold)
endif

ifeq ($(CONFIG_HAL_CAMERA_8x25Q),y)
SUBDIR_MAKEFILES += $(call all-named-subdir-makefiles,8x25q_camera)
endif

ifeq ($(CONFIG_HAL_USERVER),y)
SUBDIR_MAKEFILES += $(call all-named-subdir-makefiles,lidbg_userver)
endif

include $(SUBDIR_MAKEFILES)

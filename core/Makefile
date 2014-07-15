include $(DBG_ROOT_PATH)/build/build.mk
include $(DBG_CORE_PATH)/core.mk

obj-m	+= lidbg_io.o
obj-m	+= lidbg_uevent.o
obj-m	+= lidbg_i2c.o
#obj-m	+= lidbg_spi.o
obj-m	+= lidbg_uart.o
obj-m	+= lidbg_key.o
obj-m	+= lidbg_touch.o
obj-m	+= lidbg_misc.o
obj-m	+= lidbg_fileserver.o
obj-m	+= lidbg_common.o
obj-m	+= lidbg_wakelock_stat.o
obj-m	+= lidbg_servicer.o
obj-m	+= lidbg_msg.o
obj-m	+= lidbg_main.o
obj-m	+= lidbg_loader.o
obj-m	+= lidbg_mem_log.o
obj-m	+= lidbg_trace_msg.o
lidbg_touch-objs :=  lidbg_ts.o lidbg_ts_event.o
lidbg_common-objs :=  lidbg_cmn.o  lidbg_mem.o lidbg_display.o lidbg_notifier.o
lidbg_fileserver-objs :=  lidbg_fileserver_main.o lidbg_fs_cmn.o lidbg_fs_keyvalue.o lidbg_fs_update.o lidbg_fs_log.o lidbg_fs_conf.o
lidbg_main-objs := lidbg.o

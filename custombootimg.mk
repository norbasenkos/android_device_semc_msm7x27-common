LOCAL_PATH := $(call my-dir)

INSTALLED_BOOTIMAGE_TARGET := $(PRODUCT_OUT)/boot.img
$(INSTALLED_BOOTIMAGE_TARGET): $(TARGET_PREBUILT_KERNEL) $(recovery_ramdisk) $(INSTALLED_RAMDISK_TARGET) $(PRODUCT_OUT)/utilities/busybox $(MKBOOTIMG) $(MINIGZIP) $(INTERNAL_BOOTIMAGE_FILES)
	$(call pretty,"Boot image: $@")
	$(hide) mkdir -p $(PRODUCT_OUT)/combinedroot/
	$(hide) cp -r $(PRODUCT_OUT)/root/* $(PRODUCT_OUT)/combinedroot/
	$(hide) cp -r $(PRODUCT_OUT)/recovery/root/sbin/* $(PRODUCT_OUT)/combinedroot/sbin/
	$(hide) $(MKBOOTFS) $(PRODUCT_OUT)/combinedroot/ > $(PRODUCT_OUT)/combinedroot.cpio
	$(hide) cat $(PRODUCT_OUT)/combinedroot.cpio | $(MINIGZIP) > $(PRODUCT_OUT)/combinedroot.fs
	$(hide) $(MKBOOTIMG) --kernel $(TARGET_PREBUILT_KERNEL) --ramdisk $(PRODUCT_OUT)/combinedroot.fs --base $(BOARD_KERNEL_BASE) --output $@
	$(hide) device/semc/msm7x27-common/releasetools/bin2elf 2 0x208000 $(TARGET_PREBUILT_KERNEL) 0x208000 0x0 $(PRODUCT_OUT)/combinedroot.fs 0x1000000 0x80000000
	$(hide) mv result.elf $(PRODUCT_OUT)/result.elf
	$(hide) device/semc/msm7x27-common/releasetools/bin2sin $(PRODUCT_OUT)/result.elf 03000000010000402001000040000000
	$(hide) mv $(PRODUCT_OUT)/result.elf.sin $(PRODUCT_OUT)/kernel.sin
	$(hide) rm $(PRODUCT_OUT)/result.elf

INSTALLED_RECOVERYIMAGE_TARGET := $(PRODUCT_OUT)/recovery.img
MKRECOVERYIMAGE_FINAL := $(HOST_OUT_EXECUTABLES)/mkyaffs2image
$(INSTALLED_RECOVERYIMAGE_TARGET): $(MKBOOTIMG) \
	$(recovery_ramdisk) \
	$(recovery_kernel)
	@echo ----- Making recovery image ------
	$(hide) cp -r device/semc/msm7x27-common/recovery/charger.tar.gz $(PRODUCT_OUT)/recovery/root/sbin/
	$(hide) cp -r device/semc/msm7x27-common/recovery/init.rc $(PRODUCT_OUT)/recovery/root/
	$(hide) cp -r device/semc/msm7x27-common/recovery/charger.sh $(PRODUCT_OUT)/recovery/root/
	$(MKBOOTIMG) $(INTERNAL_RECOVERYIMAGE_ARGS) --output $@
	@echo ----- Made recovery image -------- $@
	$(hide) $(call assert-max-image-size,$@,$(BOARD_RECOVERYIMAGE_PARTITION_SIZE),raw)
	@echo ----- Making recovery image final ------
	$(hide) $(MKRECOVERYIMAGE_FINAL) $(PRODUCT_OUT)/recovery/root/ $(PRODUCT_OUT)/recovery_final.img
	@echo ----- Made recovery image final -------- $@

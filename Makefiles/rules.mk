all: $(OS_IMAGE)

include Makefiles/boot_1/boot_1_rules.mk
include Makefiles/boot_2/boot_2_rules.mk
include Makefiles/pm_entry/pm_entry_rules.mk
include Makefiles/kernel/kernel_rules.mk
include Makefiles/os_image/os_image_rules.mk

.PHONY: run clean

run:
	$(QEMU) -drive format=raw,file=$(OS_IMAGE) 

clean:
	rm -f obj/*
	rm -f bin/*
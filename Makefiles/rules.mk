all: $(OS_IMAGE)

$(BOOT_STAGE_1_OBJ): $(BOOT_STAGE_1)
	$(AS) $(BOOT_STAGE_1) -o $(BOOT_STAGE_1_OBJ)

$(BOOT_STAGE_1_BIN): $(BOOT_STAGE_1_OBJ)
	$(LD) -Ttext 0x7C00 --oformat binary $(BOOT_STAGE_1_OBJ) -o $(BOOT_STAGE_1_BIN)

$(BOOT_STAGE_2_OBJ): $(BOOT_STAGE_2)
	$(AS) $(BOOT_STAGE_2) -o $(BOOT_STAGE_2_OBJ)

$(BOOT_STAGE_2_TEMP_BIN): $(BOOT_STAGE_2_OBJ)
	$(LD) -Ttext 0x7E00 --oformat binary $(BOOT_STAGE_2_OBJ) -o $(BOOT_STAGE_2_TEMP_BIN)

$(BOOT_STAGE_2_BIN): $(BOOT_STAGE_2_TEMP_BIN)
	size=$$(wc -c < $(BOOT_STAGE_2_TEMP_BIN)); \
	if [ $$size -gt 2048 ]; then \
		echo "Stage 2 is too large."; \
		exit 1; \
	fi;	\
	cp $(BOOT_STAGE_2_TEMP_BIN) $(BOOT_STAGE_2_BIN); \
	truncate -s 2048 $(BOOT_STAGE_2_BIN)
	rm -f $(BOOT_STAGE_2_TEMP_BIN)

$(OS_IMAGE): $(BOOT_STAGE_1_BIN) $(BOOT_STAGE_2_BIN)

	cat $(BOOT_STAGE_1_BIN) $(BOOT_STAGE_2_BIN) > $(OS_IMAGE)

.PHONY: run clean

run:
	$(QEMU) -drive format=raw,file=$(OS_IMAGE) 

clean:
	rm -f obj/*
	rm -f bin/*
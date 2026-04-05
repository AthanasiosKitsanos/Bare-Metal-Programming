$(BOOT_STAGE_2_OBJ): $(BOOT_STAGE_2)
	$(AS) $(BOOT_STAGE_2) -o $(BOOT_STAGE_2_OBJ)

$(BOOT_STAGE_2_ELF): $(BOOT_STAGE_2_OBJ) $(PM_ENTRY_OBJ) $(KERNEL_OBJ)
	$(LD) -T $(BOOT_2_LINKER) -o $(BOOT_STAGE_2_ELF) \
		$(BOOT_STAGE_2_OBJ) $(PM_ENTRY_OBJ) $(KERNEL_OBJ)

$(BOOT_STAGE_2_BIN): $(BOOT_STAGE_2_ELF)
	$(OBJC) -O binary $(BOOT_STAGE_2_ELF) $(BOOT_STAGE_2_BIN)
	size=$$(wc -c < $(BOOT_STAGE_2_BIN)); \
	if [ $$size -gt 2048 ]; then \
		echo "Stage 2 is too large: $$size bytes (max 2048)."; \
		exit 1; \
	fi
	truncate -s 2048 $(BOOT_STAGE_2_BIN)
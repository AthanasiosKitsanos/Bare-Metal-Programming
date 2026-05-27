# Boot 1
$(BOOT_STAGE_1_OBJ): $(BOOT_STAGE_1) $(BOOT_STAGE_2_INC)
	$(AS) -I inc $(BOOT_STAGE_1) -o $(BOOT_STAGE_1_OBJ)

$(BOOT_STAGE_1_ELF): $(BOOT_STAGE_1_OBJ) 
	$(LD) -T $(BOOT_1_LIKNER) -o $(BOOT_STAGE_1_ELF) $(BOOT_STAGE_1_OBJ)

$(BOOT_STAGE_1_BIN): $(BOOT_STAGE_1_ELF)
	$(OBJC) -O binary $(BOOT_STAGE_1_ELF) $(BOOT_STAGE_1_BIN)

# Boot 2
$(BOOT_STAGE_2_OBJ): $(BOOT_STAGE_2)
	$(AS) $(BOOT_STAGE_2) -o $(BOOT_STAGE_2_OBJ)

# PM Entry
$(PM_ENTRY_OBJ): $(PM_ENTRY)
	$(AS) $(PM_ENTRY) -o $(PM_ENTRY_OBJ)

# Loading Configuration
$(BOOT_STAGE_2_TOTAL_SECTORS): $(CODE_32_BIN)
	size=$$(wc -c < $(CODE_32_BIN)); \
	sectors=$$(( $$size / $(SECTOR_SIZE) )); \
	if [ -z "$$sectors" ]; then \
		echo "Failed to compute stage 2 sector count."; \
		exit 1; \
	fi; \
	printf '.set BOOT_STAGE_2_SECTORS, %s\n' "$$sectors" > $(BOOT_STAGE_2_INC)
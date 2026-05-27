# Boot 1
$(BOOT_STAGE_1_OBJ): $(BOOT_STAGE_1) $(STAGE_2_SECTORS)
	$(AS) $(INCLUDE_INC_FOLDER) $(BOOT_STAGE_1) -o $(BOOT_STAGE_1_OBJ)

# Boot 2
$(BOOT_STAGE_2_OBJ): $(BOOT_STAGE_2)
	$(AS) $(BOOT_STAGE_2) -o $(BOOT_STAGE_2_OBJ)

# PM Entry
$(PM_ENTRY_OBJ): $(PM_ENTRY)
	$(AS) $(PM_ENTRY) -o $(PM_ENTRY_OBJ)

# Loading Configuration
$(STAGE_2_SECTORS): $(CODE_32_BIN)
	size=$$(wc -c < $(CODE_32_BIN)); \
	sectors=$$(( $$size / $(SECTOR_SIZE) )); \
	if [ -z "$$sectors" ]; then \
		echo "Failed to compute stage 2 sector count."; \
		exit 1; \
	fi; \
	printf '.set BOOT_STAGE_2_SECTORS, %s\n' "$$sectors" > $(STAGE_2_SECTORS)
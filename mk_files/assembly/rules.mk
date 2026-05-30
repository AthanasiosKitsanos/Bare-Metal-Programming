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

# Common Interrupt Entry
$(INTERRUPT_ENTRY_OBJ): $(INTERRUPT_ENTRY_S) 
	$(AS) $(INTERRUPT_ENTRY_S) -o $(INTERRUPT_ENTRY_OBJ)

# Code 16
$(CODE_16_ELF): $(BOOT_STAGE_1_OBJ) 
	$(LD) -T $(CODE_16_LIKNER) -o $(CODE_16_ELF) $(BOOT_STAGE_1_OBJ)

$(CODE_16_BIN): $(CODE_16_ELF)
	$(OBJC) -O binary $(CODE_16_ELF) $(CODE_16_BIN)

# Code 32
$(CODE_32_ELF): $(BOOT_STAGE_2_OBJ) $(INTERRUPT_ENTRY_OBJ) $(PM_ENTRY_OBJ) $(KERNEL_A)
	$(LD) -T $(CODE_32_LINKER) -o $(CODE_32_ELF) \
		$(BOOT_STAGE_2_OBJ) $(INTERRUPT_ENTRY_OBJ) $(PM_ENTRY_OBJ) $(LINK_ALL_LIBS)

$(CODE_32_BIN): $(CODE_32_ELF)
	$(OBJC) -O binary $(CODE_32_ELF) $(CODE_32_BIN)
	size=$$(wc -c < $(CODE_32_BIN) ); \
	sectors=$$(( ($$size + $(SECTOR_SIZE) - 1) / $(SECTOR_SIZE) )); \
	padded_size=$$(( $$sectors * $(SECTOR_SIZE) )); \
	if [ $$sectors -gt 127 ]; then \
		echo "Stage 2 is too large: $$sectors sectors (BIOS read limit exceeded)."; \
		exit 1; \
	fi; \
	truncate -s $$padded_size $(CODE_32_BIN)

# Os Image
$(OS_IMAGE): $(CODE_16_BIN) $(CODE_32_BIN)
	cat $(CODE_16_BIN) $(CODE_32_BIN) > $(OS_IMAGE)
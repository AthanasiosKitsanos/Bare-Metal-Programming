include assembly/boot/boot_mk_files/boot_rules.mk
include assembly/exception_stubs/exceptions_mk_files/exceptions_rules.mk

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

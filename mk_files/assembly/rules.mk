# Boot 1
$(BOOT_STAGE_1_OBJ): $(BOOT_STAGE_1) $(STAGE_2_SECTORS)
	$(AS) -I assembly/internal $(BOOT_STAGE_1) -o $(BOOT_STAGE_1_OBJ)

# Boot 2
$(BOOT_STAGE_2_OBJ): $(BOOT_STAGE_2)
	$(AS) $(BOOT_STAGE_2) -o $(BOOT_STAGE_2_OBJ)

# Move Kernel
$(MOVE_KERNEL_OBJ): $(MOVE_KERNEL_CPP) $(COPY_H)
	$(CC) $(COMPILE_FLAGS) -c $(MOVE_KERNEL_CPP) -o $(MOVE_KERNEL_OBJ)

# Kerne Entry
$(KERNEL_ENTRY_OBJ): $(KENREL_ENTRY)
	$(AS) $(KENREL_ENTRY) -o $(KERNEL_ENTRY_OBJ)

# PM Entry
$(PM_ENTRY_OBJ): $(PM_ENTRY) $(MOVE_KERNEL_OBJ)
	$(AS) $(PM_ENTRY) -o $(PM_ENTRY_OBJ)

# Loading Configuration
$(STAGE_2_SECTORS): $(CODE_32_BIN) $(KERNEL_BIN)
	size=$$(wc -c < $(CODE_32_BIN)); \
	size_k=$$(wc -c < $(KERNEL_BIN)); \
	total_size=$$(($$size + $$size_k)); \
	sectors=$$(( $$total_size / $(SECTOR_SIZE) )); \
	if [ $$sectors -gt 127 ]; then \
		echo "Stage_2_SECTORS - Total Sectors exceeded: $$sectors sectors (BIOS read limit exceeded)."; \
		exit 1; \
	fi; \
	printf '.set BOOT_STAGE_2_SECTORS, %s\n' "$$sectors" > $(STAGE_2_SECTORS)

# Common Interrupt Entry
$(INTERRUPT_ENTRY_OBJ): $(INTERRUPT_ENTRY_S) 
	$(AS) $(INTERRUPT_ENTRY_S) -o $(INTERRUPT_ENTRY_OBJ)

# Code 16
$(CODE_16_ELF): $(BOOT_STAGE_1_OBJ) 
	$(LD) $(LINKING_FLAGS) -T $(CODE_16_LIKNER) -o $(CODE_16_ELF) $(BOOT_STAGE_1_OBJ)

$(CODE_16_BIN): $(CODE_16_ELF)
	$(OBJC) -O binary $(CODE_16_ELF) $(CODE_16_BIN)

# Code 32
$(KERNEL_ELF): $(KERNEL_ENTRY_OBJ) $(CI_FILES_FOLDER) $(LIBRARIES) $(MAIN_OBJ)
	kernel_stack=$$(grep -v '^#' $(CALC_RESULT_FILE) | head -n 1) &&	\
	interrupt_stack=$$(grep -v '^#' $(CALC_RESULT_FILE) | tail -n 1);	\
	$(LD) $(LINKING_FLAGS) -T $(KERNEL_LINKING_FLAG) $(DEF_KERNEL_STACK)=$$kernel_stack $(DEF_INTERRUPT_STACK)=$$interrupt_stack -o $(KERNEL_ELF) \
		$(INTERRUPT_ENTRY_OBJ) $(KERNEL_ENTRY_OBJ) $(MAIN_OBJ) \
		$(LINK_LIBRARIES)

$(KERNEL_DISASM): $(KERNEL_ELF)
	$(OBJDUMP) -d $(KERNEL_ELF) > $(KERNEL_DISASM)

$(KERNEL_BIN): $(KERNEL_ELF) $(KERNEL_DISASM)
	$(OBJC) -O binary $(KERNEL_ELF) $(KERNEL_BIN)
	size=$$(wc -c < $(KERNEL_BIN) ); \
	sectors=$$(( ($$size + $(SECTOR_SIZE) - 1) / $(SECTOR_SIZE) )); \
	padded_size=$$(( $$sectors * $(SECTOR_SIZE) )); \
	truncate -s $$padded_size $(KERNEL_BIN)
	
$(CODE_32_ELF): $(KERNEL_ELF)
	$(LD) $(LINKING_FLAGS) -T $(CODE_32_LINKER) --just-symbols=$(KERNEL_ELF) -o $(CODE_32_ELF) \
		$(BOOT_STAGE_2_OBJ) $(PM_ENTRY_OBJ) $(MOVE_KERNEL_OBJ)
	
$(CODE_32_DISASM): $(CODE_32_ELF)
	$(OBJDUMP) -d $(CODE_32_ELF) > $(CODE_32_DISASM)

$(CODE_32_BIN): $(CODE_32_ELF) $(CODE_32_DISASM)
	$(OBJC) -O binary $(CODE_32_ELF) $(CODE_32_BIN)
	size=$$(wc -c < $(CODE_32_BIN) ); \
	sectors=$$(( ($$size + $(SECTOR_SIZE) - 1) / $(SECTOR_SIZE) )); \
	padded_size=$$(( $$sectors * $(SECTOR_SIZE) )); \
	truncate -s $$padded_size $(CODE_32_BIN)

# Os Image
$(OS_IMAGE): $(CODE_16_BIN) $(CODE_32_BIN) $(KERNEL_BIN)
	cat $(CODE_16_BIN) $(CODE_32_BIN) $(KERNEL_BIN) > $(OS_IMAGE)
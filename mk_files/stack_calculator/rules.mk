$(CI_FILES): $(BOOT_STAGE_2_OBJ) $(INTERRUPT_ENTRY_OBJ) $(PM_ENTRY_OBJ) $(MAIN_OBJ) $(LIBRARIES)
	find obj -name '*.ci' -exec sh -c 'mv "$$@" ci_files' sh {} +
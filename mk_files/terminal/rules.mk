# Terminal Output
$(OUTPUT_OBJ): $(VGA_H) $(TERMINAL_H) $(OUTPUT_CPP) $(IO_H) $(LOGGER_H)
	$(CC) $(COMPILE_FLAGS) -c $(OUTPUT_CPP) -o $(OUTPUT_OBJ)

# Terminal Input
$(INPUT_OBJ): $(INPUT_CPP) $(INPUT_H)
	$(CC) $(COMPILE_FLAGS) -c $(INPUT_CPP) -o $(INPUT_OBJ)
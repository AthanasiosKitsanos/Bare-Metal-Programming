include assembly/boot/boot_mk_files/boot_declarations.mk
include assembly/exception_stubs/exceptions_mk_files/exceptions_declarations.mk

#----------------------Boot Code 16 -----------------------------
CODE_16_ELF = elf/code_16.elf
CODE_16_BIN = bin/code_16.bin
CODE_16_LIKNER = links/code_16.ld

# --------------------Code 32--------------------------------
CODE_32_ELF = elf/code_32.elf
CODE_32_BIN = bin/code_32.bin
CODE_32_LINKER = links/code_32.ld

# ------------------------OS Image---------------------------
OS_IMAGE = bin/os_image.bin
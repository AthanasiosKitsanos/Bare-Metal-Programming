CC = i686-elf-g++

AS = i686-elf-as
LD = i686-elf-ld

QEMU = qemu-system-x86_64

CC_FLAGS = -std=gnu++17 -ffreestanding -O3 -Wall -Wextra -fno-exceptions -fno-rtti
LINKING_FLAGS = -ffreestanding -O3 -nostdlib

POWER_SHELL = powershell -NoProfile -Command 
COMMANDS = $$bytes = [System.IO.File]::ReadAllBytes('bin\boot_stage_2.bin'); $$target_size = 2048; if($$bytes.Length -gt $$target_size) { throw "Stage 2 is too large." }; $$padded = New-Object byte[] $$target_size; [Array]::Copy($$bytes, $$padded, $$bytes.Length); [System.IO.File]::WriteAllBytes('bin\boot_stage_2_padded.bin', $$padded)
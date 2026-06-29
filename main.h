#pragma once

#include <stdint.h>
#include "io/output/terminal_output.h"
#include "io/input/terminal_input.h"
#include "logger/kernel_logger.h"
#include "exceptions/kernel_exceptions.h"
#include "timer/kernel_timer.h"
#include "pit/kernel_pit.h"
#include "keyboard/keyboard.h"
#include "internal/kernel_interrupt_guard.h"
#include "memory/e820/kernel_e820.h"
#include "memory/pmm/kernel_pmm.h"
#include "shell/shell.h"
#include "utilities/cpu/features.h"
#include "memory/heap/kernel_heap.h"
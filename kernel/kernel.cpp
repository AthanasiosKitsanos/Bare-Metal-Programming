#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#if defined(__linux__)
#error "You are not using a cross-compiler, you will most certainly run into trouble"
#endif

enum vga_color: uint8_t
{
    VGA_COLOR_BLACK = 0,
	VGA_COLOR_BLUE = 1,
	VGA_COLOR_GREEN = 2,
	VGA_COLOR_CYAN = 3,
	VGA_COLOR_RED = 4,
	VGA_COLOR_MAGENTA = 5,
	VGA_COLOR_BROWN = 6,
	VGA_COLOR_LIGHT_GREY = 7,
	VGA_COLOR_DARK_GREY = 8,
	VGA_COLOR_LIGHT_BLUE = 9,
	VGA_COLOR_LIGHT_GREEN = 10,
	VGA_COLOR_LIGHT_CYAN = 11,
	VGA_COLOR_LIGHT_RED = 12,
	VGA_COLOR_LIGHT_MAGENTA = 13,
	VGA_COLOR_LIGHT_BROWN = 14,
	VGA_COLOR_WHITE = 15
};

static inline uint8_t __attribute__((always_inline)) vga_entry_color(vga_color fg, vga_color bg)
{
    return fg | bg << 4;
}

static inline uint16_t __attribute__((always_inline)) vga_entry(unsigned char uc, uint8_t color)
{
    return (uint16_t)uc | color << 8;
}

static inline void __attribute__((always_inline)) outb(uint16_t port, uint8_t value)
{
    asm volatile ("outb %0, %1": : "a"(value), "Nd"(port));
}

size_t strlen(const char* str)
{
    size_t len{0};
    while(*(str + len)) ++len;
    return len;
}



#define VGA_WIDTH  80
#define VGA_HEIGHT 25
#define VGA_MEMORY 0xB8000

uint8_t terminal_color;
volatile uint16_t* terminal_buffer;
volatile uint16_t* cursor;

void update_hardware_cursor()
{
    const size_t position{static_cast<size_t>(cursor - terminal_buffer)};
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(position & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((position >> 8) & 0xFF));
}

inline void __attribute__((always_inline)) reset_cursor() { cursor = terminal_buffer; }

void terminal_initialize()
{
    terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    terminal_buffer = (volatile uint16_t*)VGA_MEMORY;
    volatile uint16_t* const buffer_end{terminal_buffer + (VGA_WIDTH * VGA_HEIGHT)};
    for(cursor = terminal_buffer; cursor < buffer_end ; ++cursor)
    {
        *cursor = vga_entry(' ', terminal_color);
    }
    reset_cursor();
}

inline void __attribute__((always_inline)) terminal_set_color(uint8_t color)
{
    terminal_color = color;
}

void terminal_write_string(const char* data)
{
    for(; *data != '\0'; ++data)
    {
        *cursor = vga_entry(*data, terminal_color);
        ++cursor;
    }
    update_hardware_cursor();
}

extern "C" void kernel_main()
{
    terminal_initialize();
    terminal_write_string("Hello from kernel World!");
}
#include "memory/heap/kernel_heap.h"

namespace
{
    kernel::memory::block_header* free_list_head{nullptr};
    kernel::memory::block_header* heap_end{nullptr};
    kernel::memory::block_header* searching_block{nullptr};
}

namespace kernel::memory
{
    void heap_initialize(void* heap_start, uint32_t heap_size) noexcept
    {
        free_list_head = reinterpret_cast<block_header*>(heap_start);
        free_list_head->next = nullptr;
        free_list_head->size = heap_size - sizeof(block_header);
        free_list_head->flags = 0;
        free_list_head->prev = nullptr;
        free_list_head->physical_prev = nullptr;
        searching_block = free_list_head;
        heap_end = reinterpret_cast<block_header*>(reinterpret_cast<uint8_t*>(heap_start) + heap_size);
    }

    [[gnu::regparm(1)]]
    void* kmalloc(uint32_t requested_size) noexcept
    {
        uint32_t cached_size{0};
        while(searching_block != nullptr)
        {
            cached_size = searching_block->size;
            if((cached_size * (~(searching_block->flags) & 1)) >= requested_size) break;
            searching_block = searching_block->next;
        }
        if(!searching_block) return nullptr;

        block_header* allocated{searching_block};
        {
            constexpr uint32_t split_limit{sizeof(block_header) + 8};
            const uint32_t remaining{cached_size - requested_size};
            if(remaining >= split_limit)
            {
                block_header* remainder_block{reinterpret_cast<block_header*>(reinterpret_cast<uint8_t*>(allocated + 1) + requested_size)};
                remainder_block->next = allocated->next;
                remainder_block->size = remaining - sizeof(block_header);
                remainder_block->flags = 0;
                remainder_block->prev = allocated;
                remainder_block->physical_prev = allocated;
                if(remainder_block->next)
                {
                    remainder_block->next->prev = remainder_block;
                    remainder_block->next->physical_prev = remainder_block;
                }
                
                allocated->next = remainder_block;
                allocated->size = requested_size;
            }
        }

        allocated->flags = 1;

        const uintptr_t next_addr{reinterpret_cast<uintptr_t>(allocated->next)};
        const bool is_null{next_addr == 0};
        searching_block = reinterpret_cast<block_header*>(next_addr * !is_null + (reinterpret_cast<uintptr_t>(free_list_head) * is_null));
        return reinterpret_cast<void*>(allocated + 1);
    }

    [[gnu::regparm(1)]]
    void kfree(void* ptr) noexcept
    {
        if(!ptr) return;
        block_header* allocated_memory{reinterpret_cast<block_header*>(ptr) - 1};
        allocated_memory->flags = 0;
        block_header* next{reinterpret_cast<block_header*>(reinterpret_cast<uint8_t*>(ptr) + allocated_memory->size)};
        if(next < heap_end)
        {
            if((next->flags & 1) == 0)
            {
                if(next->prev) next->prev->next = next->next;
                if(next->next) next->next->prev = next->prev;
                if(next == free_list_head) free_list_head = next->next;
                if(next == searching_block) searching_block = next->next;
                allocated_memory->size += (sizeof(block_header) + next->size);
            }
        }
        allocated_memory->next = free_list_head;
        allocated_memory->prev = nullptr;
        if(free_list_head) free_list_head->prev = allocated_memory;
        free_list_head = allocated_memory;
        
        searching_block = allocated_memory;
    }

    [[gnu::regparm(2)]]
    void* krealloc(void* ptr, const uint32_t new_size) noexcept
    {
        if(!ptr) return kmalloc(new_size);
        if(new_size == 0)
        {
            kfree(ptr);
            return nullptr;
        }

        block_header* reallocate{reinterpret_cast<block_header*>(ptr) - 1};
        if(reallocate->size >= new_size) return ptr;

        void* new_ptr{kmalloc(new_size)};
        if(!new_ptr) return nullptr;

        uint8_t* source{reinterpret_cast<uint8_t*>(ptr)};
        uint8_t* destination{reinterpret_cast<uint8_t*>(new_ptr)};

        const bool is_lower{new_size < reallocate->size};
        const uint32_t length{new_size * is_lower + reallocate->size * !is_lower};
        for(uint32_t i{0}; i < length; ++i)
        {
            *destination = *source;
            ++destination;
            ++source;
        }

        kfree(ptr);
        return new_ptr;
    }
}
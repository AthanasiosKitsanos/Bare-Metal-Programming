#include "memory/heap/kernel_heap.h"

namespace
{
    kernel::memory::block_header* free_list_head{nullptr};
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
        searching_block = free_list_head;
    }

    void* kmalloc(uint32_t requested_size) noexcept
    {
        uint32_t cached_size{0};
        while(searching_block != nullptr)
        {
            cached_size = searching_block->size;
            if((cached_size * (~(searching_block->flags) & 1)) >= requested_size) break;
            searching_block = searching_block->next;
        }
        if(searching_block == nullptr) return nullptr;

        block_header* allocated{searching_block};
        {
            constexpr uint32_t split_limit{sizeof(block_header) + 8};
            const uint32_t remaining{cached_size - requested_size};
            if(remaining >= split_limit)
            {
                block_header* block_p1{reinterpret_cast<block_header*>(reinterpret_cast<uint8_t*>(allocated + 1) + requested_size)};
                block_p1->next = allocated->next;
                block_p1->size = remaining - sizeof(block_header);
                block_p1->flags = 0;

                allocated->next = block_p1;
                allocated->size = requested_size;
            }
        }

        allocated->flags = 1;

        const uintptr_t next_addr{reinterpret_cast<uintptr_t>(allocated->next)};
        const bool is_null{next_addr == 0};
        searching_block = reinterpret_cast<block_header*>(next_addr * !is_null + (reinterpret_cast<uintptr_t>(free_list_head) * is_null));
        return reinterpret_cast<void*>(allocated + 1);
    }

    void kfree(void* ptr) noexcept
    {
        if(!ptr) return;
        block_header* allocated_memory{reinterpret_cast<block_header*>(ptr) - 1};
        allocated_memory->flags = 0;
        const bool is_lower{allocated_memory < searching_block};
        searching_block = reinterpret_cast<block_header*>(reinterpret_cast<uintptr_t>(free_list_head) * !is_lower + reinterpret_cast<uintptr_t>(allocated_memory) * is_lower);
    }
}
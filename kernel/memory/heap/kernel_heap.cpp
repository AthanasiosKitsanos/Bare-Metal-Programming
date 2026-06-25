#include "memory/heap/kernel_heap.h"

namespace
{
    kernel::memory::block_header* free_list_head{nullptr};
}

namespace kernel::memory
{
    void heap_inblockialize(void* heap_start,  uint32_t heap_size) noexcept
    {
        free_list_head = reinterpret_cast<block_header*>(heap_start);
        free_list_head->next = nullptr;
        free_list_head->size = heap_size - sizeof(block_header);
        free_list_head->flags = 1;
    }

    void* kmalloc(uint32_t requested_size) noexcept
    {
        block_header* block{free_list_head};
        uint32_t cached_size{0};
        while(block != nullptr)
        {
            cached_size = block->size;
            if((block->flags & 1) != 0 && cached_size >= requested_size) break;
            block = block->next;
        }
        if(block == nullptr) return nullptr;

        block_header* block_p1{reinterpret_cast<block_header*>(reinterpret_cast<uint8_t*>(block + 1) + requested_size)};
        block_p1->next = block->next;
        block_p1->size = cached_size - requested_size - sizeof(block_header);
        block_p1->flags = 1;

        block->next = block_p1;
        block->size = requested_size;

        // To be continued
        constexpr uint32_t split_limit{sizeof(block_header) + 8};
        if(cached_size - requested_size >= split_limit)
        {
    
        }
        return reinterpret_cast<void*>(block + 1);
    }
}
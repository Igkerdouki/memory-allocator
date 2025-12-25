#include "vm.h"
#include "vmlib.h"

void *vmalloc(size_t size)
{
    // 1. Zero-sized allocation → no-op
    if (size == 0) {
        return NULL;
    }

    // 2. Compute required block size (header + payload), 16-byte aligned
    //    NOTE: block size = ROUND_UP(size + header, 16)
    size_t needed_block_size =
        ROUND_UP(size + sizeof(struct block_header), BLKALIGN);

    // 3. Best-fit search across the heap
    struct block_header *curr = heapstart;
    struct block_header *best = NULL;
    size_t best_size = 0;

    while (curr->size_status != VM_ENDMARK) {
        size_t curr_size = BLKSZ(curr);
        int busy = curr->size_status & VM_BUSY;

        if (!busy && curr_size >= needed_block_size) {
            if (best == NULL || curr_size < best_size) {
                best = curr;
                best_size = curr_size;
            }
        }

        curr = (struct block_header *)((char *)curr + curr_size);
    }

    // 4. No suitable block found
    if (best == NULL) {
        return NULL;
    }

    // 5. We found a best-fit free block
    struct block_header *alloc = best;
    size_t prevbit = alloc->size_status & VM_PREVBUSY;
    size_t remainder = best_size - needed_block_size;

    if (remainder >= 16) {
        //  SPLIT

        // front chunk: allocated block
        size_t alloc_size = needed_block_size;
        alloc->size_status = alloc_size | prevbit | VM_BUSY;

        // back chunk: new free block
        struct block_header *freeblk =
            (struct block_header *)((char *)alloc + alloc_size);
        size_t free_size = remainder;

        // header for the free block: free, prev is busy
        freeblk->size_status = free_size | VM_PREVBUSY;

        // footer for the free block at the end of that block
        struct block_footer *footer =
            (struct block_footer *)((char *)freeblk + free_size
                                    - sizeof(struct block_footer));
        footer->size = free_size;

        // fix next block's PREVBUSY bit (previous is now FREE)
        struct block_header *next =
            (struct block_header *)((char *)freeblk + free_size);
        if (next->size_status != VM_ENDMARK) {
            next->size_status &= ~VM_PREVBUSY;
        }

    } else {
        //NO SPLIT use entire block
        size_t alloc_size = best_size;
        alloc->size_status = alloc_size | prevbit | VM_BUSY;

        struct block_header *next =
            (struct block_header *)((char *)alloc + alloc_size);
        if (next->size_status != VM_ENDMARK) {
            next->size_status |= VM_PREVBUSY;
        }
    }

    // 6. Return pointer to payload
    return (void *)((char *)alloc + sizeof(struct block_header));
}


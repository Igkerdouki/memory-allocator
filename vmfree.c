#include "vm.h"
#include "vmlib.h"

/**
 * The vmfree() function frees the memory space pointed to by ptr,
 * which must have been returned by a previous call to vmmalloc().
 * Otherwise, or if free(ptr) has already been called before,
 * undefined behavior occurs.
 * If ptr is NULL, no operation is performed.
 */
void vmfree(void *ptr)
{
    if (ptr ==NULL){
		return;
}
// 2. Get the block header from the payload pointer
    struct block_header *hdr =
        (struct block_header *)((char *)ptr - sizeof(struct block_header));

    // 3. If block is already free, do nothing
    if (!(hdr->size_status & VM_BUSY)) {
        return;
    }

    // 4. Get size of this block and mark it free (keep PREVBUSY bit)
    size_t size = BLKSZ(hdr);
    hdr->size_status = size | (hdr->size_status & VM_PREVBUSY);

    // 5. Write footer for this (now free) block
    struct block_footer *footer =
        (struct block_footer *)((char *)hdr + size - sizeof(struct block_footer));
    footer->size = size;

    // 6.I am  coalescing with previous block if it is free
    if (!(hdr->size_status & VM_PREVBUSY)) {
        // Previous block is free: its footer is just before hdr
        struct block_footer *prev_footer =
            (struct block_footer *)((char *)hdr - sizeof(struct block_footer));
        size_t prev_size = prev_footer->size;

        struct block_header *prev_hdr =
            (struct block_header *)((char *)hdr - prev_size);

        // Merge prev + current
        size += prev_size;
        hdr = prev_hdr;

        hdr->size_status = size | (hdr->size_status & VM_PREVBUSY);

        // Update footer of the merged block
        footer = (struct block_footer *)((char *)hdr + size - sizeof(struct block_footer));
        footer->size = size;
    }

    // I am  Try coalescing with next block if it is free
    struct block_header *next =
        (struct block_header *)((char *)hdr + size);

    if (next->size_status != VM_ENDMARK && !(next->size_status & VM_BUSY)) {
        size_t next_size = BLKSZ(next);
        size += next_size;

        hdr->size_status = size | (hdr->size_status & VM_PREVBUSY);

        footer = (struct block_footer *)((char *)hdr + size - sizeof(struct block_footer));
        footer->size = size;

        // recompute next after merging
        next = (struct block_header *)((char *)hdr + size);
    }

    // 8. Fix PREVBUSY bit of the block after the final merged block
    if (next->size_status != VM_ENDMARK) {
        //  block (hdr) is free, so clear PREVBUSY of next
        next->size_status &= ~VM_PREVBUSY;
    }
}

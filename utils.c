#include <stdio.h>

#include "vm.h"
#include "vmlib.h"

#define BLKOFF(b) ((char *)b - (char *)heapstart + sizeof(struct block_header))

void vminfo()
{
    static const char *blkstats[] = {"FREE", "BUSY"};

    struct block_header *block = heapstart;
    int blockid = 0;
    int nfree = 0, nbusy = 0;

    int busy, prev_busy;
    size_t blocksz;
    size_t totalsz = 0;


	 int largest_index = -1;
	 size_t largest_size =0;

    printf("---------------------------------------\n");
    printf(" %-6s %-7s %-8s %-8s %-7s\n", "#", "stat", "offset", "size",
           "prev");
    printf("---------------------------------------\n");
    while (block->size_status != VM_ENDMARK) {
        blocksz = BLKSZ(block);
        busy = block->size_status & VM_BUSY ? 1 : 0;
        prev_busy = block->size_status & VM_PREVBUSY ? 1 : 0;
        
       //update largest free block before changing blockid
        if (!busy) {
			if (blocksz>largest_size){
				largest_size = blocksz;
				largest_index = blockid;
			}
		}
    printf(" %-6d %-7s %-8zu %-8zu %-7s\n", blockid, blkstats[busy],
               BLKOFF(block), blocksz, blkstats[prev_busy]);

        if (busy) {
            nbusy++;
        } else {
            nfree++;
        }
        totalsz += blocksz;

        block = (struct block_header *)((char *)block + blocksz);
        blockid++;
    }
    printf(" %-6s %-7s %-8zu %-8s %-7s\n", "END", "N/A", BLKOFF(block), "N/A",
           "N/A");
    printf("---------------------------------------\n");
    printf("Total: %zu bytes, Free: %d, Busy: %d, Total: %d\n", totalsz, nfree,
           nbusy, nfree + nbusy);

    /* print largest free block info (if any free blocks exist) */
    if (largest_index >= 0) {
        printf("The largest free block is #%d with size %zu\n",
               largest_index, largest_size);
    }
}

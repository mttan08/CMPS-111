#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "directory.h"
#include "superblock.h"
#include "fsbase.h"

int main(int argc, char const *argv[]) {
    FILE* file;
    FileSystemBase fsb;
    unsigned int num_blocks = atoi(argv[1]);
    unsigned int blk_size = atoi(argv[2]);
    file = fopen("disk.img", "wb+");

    int faBlocksNeeded = (4 * num_blocks) / blk_size;
    if ((4 * num_blocks) % blk_size) ++faBlocksNeeded;

    printf("Number of blocks: %d, block size: %d, fa blocks: %d\n", num_blocks, blk_size, faBlocksNeeded);

    fsb_init(&fsb, file, num_blocks, faBlocksNeeded, blk_size);
    fclose(file);
    return 0;
}

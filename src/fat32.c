#include "lib-header/stdtype.h"
#include "lib-header/fat32.h"
#include "lib-header/stdmem.h"

const uint8_t fs_signature[BLOCK_SIZE] = {
    'C', 'o', 'u', 'r', 's', 'e', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',  ' ',
    'D', 'e', 's', 'i', 'g', 'n', 'e', 'd', ' ', 'b', 'y', ' ', ' ', ' ', ' ',  ' ',
    'L', 'a', 'b', ' ', 'S', 'i', 's', 't', 'e', 'r', ' ', 'I', 'T', 'B', ' ',  ' ',
    'M', 'a', 'd', 'e', ' ', 'w', 'i', 't', 'h', ' ', '<', '3', ' ', ' ', ' ',  ' ',
    '-', '-', '-', '-', '-', '-', '-', '-', '-', '-', '-', '2', '0', '2', '3', '\n',
    [BLOCK_SIZE-2] = 'O',
    [BLOCK_SIZE-1] = 'k',
};

struct FAT32FileAllocationTable fat32_allocation_table = {
    // fill all with FAT32_FAT_EMPTY_ENTRY
    .cluster_map = {FAT32_FAT_EMPTY_ENTRY},
    // cluster 0, cluster 1, and root is always intialized
    .cluster_map[0] = (uint32_t) CLUSTER_0_VALUE,
    .cluster_map[1] = (uint32_t) CLUSTER_1_VALUE,
    .cluster_map[2] = (uint32_t) FAT32_FAT_END_OF_FILE,
};
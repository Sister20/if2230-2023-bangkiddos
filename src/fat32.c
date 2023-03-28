#include "lib-header/stdtype.h"
#include "lib-header/fat32.h"
#include "lib-header/stdmem.h"
#include "lib-header/disk.h"

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

struct FAT32DriverState driver_state;

void initialize_filesystem_fat32(void) {
    if (is_empty_storage()) {
        create_fat32();
    } else {
        read_clusters(&driver_state.fat_table, 1, 1);
    }
}

struct BlockBuffer boot_sector;

bool is_empty_storage(void) {
    read_blocks(&boot_sector, 0, 1); // read boot sector from block 0
    return (memcmp(&boot_sector, fs_signature, BLOCK_SIZE) != 0);
}

void create_fat32(void) {
    // Write fs_signature to boot sector
    write_blocks(&boot_sector, 0, 1);

    // Initialize FileAllocationTable (FAT)
    struct FAT32FileAllocationTable fat_table = fat32_allocation_table;
    write_clusters(&fat_table, 1, 1);

    // Initialize DirectoryTable
    struct FAT32DirectoryTable dir_table;
    init_directory_table(&dir_table, "bangkiddOS", 0);
    write_clusters(&dir_table, 2, 1);
}

/* 4 Blocks per Cluster*/

void write_clusters(const void *ptr, uint32_t cluster_number, uint8_t cluster_count) {
    uint32_t block_number = cluster_number * 4;
    uint8_t block_count = cluster_count * 4;

    write_blocks(ptr, block_number, block_count);
}

void read_clusters(void *ptr, uint32_t cluster_number, uint8_t cluster_count) {
    uint32_t block_number = cluster_number * 4;
    uint8_t block_count = cluster_count * 4;

    read_blocks(ptr, block_number, block_count);
}
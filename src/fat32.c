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
    // .cluster_map = {FAT32_FAT_EMPTY_ENTRY},
    // cluster 0, cluster 1, and root is always intialized
    .cluster_map[0] = (uint32_t) CLUSTER_0_VALUE,
    .cluster_map[1] = (uint32_t) CLUSTER_1_VALUE,
    .cluster_map[2] = (uint32_t) FAT32_FAT_END_OF_FILE,
};

struct FAT32DriverState driver_state = {0};

void initialize_filesystem_fat32(void) {
    if (is_empty_storage()) {
        create_fat32();
    } else {
        read_clusters(&driver_state.fat_table, 1, 1);
    }
}

bool is_empty_storage(void) {
    struct ClusterBuffer read = {0};

    read_blocks(&read, BOOT_SECTOR, 1); // read boot sector from block 0
    return (memcmp(&read, fs_signature, BLOCK_SIZE) != 0);
}

void create_fat32(void) {
    // Write fs_signature to boot sector
    write_blocks(fs_signature, BOOT_SECTOR, 1);

    // Initialize FileAllocationTable (FAT)
    struct FAT32FileAllocationTable fat_table = fat32_allocation_table;
    write_clusters(&fat_table, 1, 1);

    // Initialize DirectoryTable
    struct FAT32DirectoryTable dir_table = {0};
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

int8_t dirtable_linear_search(uint32_t parent_cluster_number, struct FAT32DriverRequest entry){
    struct FAT32DirectoryTable directory_parent;
    read_clusters(&directory_parent, parent_cluster_number, 1);

    // iterate over each directory entry in the table
    for(int i=0; i<64; i++){
        char* current_name = directory_parent.table[i].name;
        char* current_ext = directory_parent.table[i].ext;
        if(!memcmp(current_name, entry.name, 8)){
            if(entry.buffer_size == 0 || !memcmp(current_ext, entry.ext, 3)){
                return i;
            }
        }
    }

    return -1; //not found
}

int8_t read(struct FAT32DriverRequest request) {
    // Search for the entry in the directory table
    int8_t foundIndex = dirtable_linear_search(request.parent_cluster_number, request);
    if (foundIndex == -1) {
        return 3; 
    }

    // Read the directory table and file allocation table
    struct FAT32DirectoryTable directory_parent;
    read_clusters(&directory_parent, request.parent_cluster_number, 1);
    struct FAT32FileAllocationTable fat;
    read_clusters(&fat, FAT_CLUSTER_NUMBER, 1);

    
    struct FAT32DirectoryEntry entry = directory_parent.table[foundIndex];
    if(request.buffer_size < entry.filesize){
        return 2;
    }
    if (entry.attribute == ATTR_SUBDIRECTORY) {
        return 1;
    }

    // Read the data clusters associated with the entry
    int32_t entry_cluster = entry.cluster_high << 16 | entry.cluster_low;
    int32_t current_cluster = entry_cluster;
    int16_t cluster_count = 0;
    int32_t next_cluster;

    do {
        read_clusters(request.buf + cluster_count * CLUSTER_SIZE, current_cluster, 1);
        next_cluster = fat.cluster_map[current_cluster];
        current_cluster = next_cluster;
        cluster_count++;
    } while (next_cluster != FAT32_FAT_END_OF_FILE);

    return 0; // Return success code
}

int my_strlen(char *str) {
	int count = 0;

    while (*str != '\0') {
        count ++;
        str ++;
    }

	return count;
}

void init_directory_table(struct FAT32DirectoryTable *dir_table, char *name, uint32_t parent_dir_cluster) {
    // set all directory entries to be empty
    for (unsigned int i = 0; i < CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry); i++) {
        memset(dir_table->table[i].name, 0, 8);
        memset(dir_table->table[i].ext, 0, 3);
        dir_table->table[i].cluster_high = 0;
        dir_table->table[i].cluster_low = 0;
        dir_table->table[i].filesize = 0;
    }

    // initialize the first directory entry with the parent directory
    for (int i = 0; i < my_strlen(name) && i < 8; i++) {
        dir_table->table[0].name[i] = name[i];
    }
    for (int i = my_strlen(name); i < 8; i++) {
        dir_table->table[0].name[i] = ' ';
    }
    for (int i = 0; i < 3; i++) {
        dir_table->table[0].ext[i] = ' ';
    }
    dir_table->table[0].attribute = ATTR_SUBDIRECTORY;
    dir_table->table[0].user_attribute = UATTR_NOT_EMPTY;
    dir_table->table[0].cluster_high = (parent_dir_cluster >> 16) & 0xFFFF;
    dir_table->table[0].cluster_low = parent_dir_cluster & 0xFFFF;
    dir_table->table[0].filesize = 0;
}

// * @return Error code: 0 success - 1 not a folder - 2 not found - -1 unknown

int8_t read_directory(struct FAT32DriverRequest request) {

}

//  * @return Error code: 0 success - 1 not a file - 2 not enough buffer - 3 not found - -1 unknown

// int8_t write(struct FAT32DriverRequest request);

// int8_t delete(struct FAT32DriverRequest request);
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
    init_directory_table(&dir_table, "test", 0);
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

int8_t dirtable_linear_search(uint32_t parent_cluster_number, struct FAT32DriverRequest entry){
    // Read directory table
    read_clusters(&driver_state.dir_table_buf, parent_cluster_number, 1);

    // Iterate over each directory entry in the table
    for(int i = 1; i < 64; i++){
        char* current_name = driver_state.dir_table_buf.table[i].name;
        char *current_ext = driver_state.dir_table_buf.table[i].ext;
        if(!memcmp(current_name, entry.name, 8)){
            if(entry.buffer_size == 0 || !memcmp(current_ext, entry.ext, 3)){
                return i;
            }
        }
    }

    return -1; // not found
}

int8_t read(struct FAT32DriverRequest request) {
    // Search for the entry in the directory table
    int8_t foundIndex = dirtable_linear_search(request.parent_cluster_number, request);
    if (foundIndex == -1) {
        return 3; // not found
    }

    // Read directory table
    read_clusters(&driver_state.dir_table_buf, request.parent_cluster_number, 1);

    // If unknown
    if (driver_state.dir_table_buf.table[0].attribute != ATTR_SUBDIRECTORY) {
        return -1;
    }
    
    // entry
    struct FAT32DirectoryEntry entry = driver_state.dir_table_buf.table[foundIndex];

    // If not enough buffer
    if(request.buffer_size < entry.filesize){
        return 2;
    }

    // If not a file
    if (entry.attribute == ATTR_SUBDIRECTORY) {
        return 1;
    }

    // Read the data clusters associated with the entry
    int32_t entry_cluster = entry.cluster_high << 16 | entry.cluster_low;
    int32_t current_cluster = entry_cluster;
    int16_t cluster_count = 0;
    int32_t next_cluster;

    do {
        read_clusters(&request.buf + cluster_count * CLUSTER_SIZE, current_cluster, 1);
        next_cluster = driver_state.fat_table.cluster_map[current_cluster];
        current_cluster = next_cluster;
        cluster_count++;
    } while (next_cluster != FAT32_FAT_END_OF_FILE);

    return 0; // Return success code
}

int8_t read_directory(struct FAT32DriverRequest request) {
    // Search for the entry in the directory table
    int8_t foundIndex = dirtable_linear_search(request.parent_cluster_number, request);
    if (foundIndex == -1) {
        return 2; // not found
    }

    // Read directory table
    read_clusters(&driver_state.dir_table_buf, request.parent_cluster_number, 1);

    // If unknown
    if (driver_state.dir_table_buf.table[0].attribute != ATTR_SUBDIRECTORY) {
        return -1;
    }

    // entry
    struct FAT32DirectoryEntry entry = driver_state.dir_table_buf.table[foundIndex];

    // If not a folder
    if (entry.attribute == ATTR_SUBDIRECTORY) { 
        return 1;
    }

    int32_t entry_cluster = entry.cluster_high << 16 | entry.cluster_low;

    read_clusters(&request.buf, entry_cluster, 1);

    return 0; // Return success code
}

int8_t delete(struct FAT32DriverRequest request) {
    // Search for the entry in the directory table
    int8_t foundIndex = dirtable_linear_search(request.parent_cluster_number, request);
    if (foundIndex == -1) {
        return 1;
    } else if (foundIndex != 0) {
        // check the number of files in the directory
        int numberOfFiles = 0;
        for(int i = 1; i < 64; i++){
            char* current_name = driver_state.dir_table_buf.table[i].name;
            char *current_ext = driver_state.dir_table_buf.table[i].ext;
            if(!memcmp(current_name, 0, 8)){
                break;
            } else {
                numberOfFiles++;
            }
        }

        if (numberOfFiles > 1) {
            return 2;
        }

        int32_t clusternumber = &driver_state.dir_table_buf.table[foundIndex].cluster_low;
        
        memset(&driver_state.dir_table_buf.table[foundIndex], 0, sizeof(struct FAT32DirectoryEntry));

        int32_t cur_cluster = clusternumber;
        int32_t cur_block = fat32_allocation_table.cluster_map[cur_cluster];
        // process of deleting the files from allocation table
        while (cur_block != FAT32_FAT_END_OF_FILE) {
            memset(&fat32_allocation_table.cluster_map[cur_cluster], 0, sizeof(int32_t));
            cur_cluster = cur_block;
            cur_block = fat32_allocation_table.cluster_map[cur_cluster];
        }
        memset(&fat32_allocation_table.cluster_map[cur_cluster], 0, sizeof(int32_t));
        write_clusters(&fat32_allocation_table, FAT_CLUSTER_NUMBER, 1);
        write_clusters(&driver_state, request.parent_cluster_number, 1);
        return 0;
    } else {
        return 2; // The index is located at index 0 in directorytable
    }

    return -1;
}

int8_t write(struct FAT32DriverRequest request) {
    // get fat table
    int currentClusterNumber = request.parent_cluster_number;
    int currentParentClusterNumber = request.parent_cluster_number;
    read_clusters((void *)&driver_state.fat_table, FAT_CLUSTER_NUMBER, 1);
    read_clusters((void *)&driver_state.dir_table_buf, currentParentClusterNumber, 1);

    // parent is valid
    if (driver_state.dir_table_buf.table[0].attribute == ATTR_SUBDIRECTORY) {
        
        // find cluster to fill table using loop in cluster map
        uint32_t clusterIndex = 3;
        while (driver_state.fat_table.cluster_map[clusterIndex] != 0 && clusterIndex< CLUSTER_MAP_SIZE) {
            clusterIndex++;
        }

        // check if file/folder already existed in parent
        bool isFound = 0;
        uint32_t entry = 0;
        uint32_t checkIndex = 0;

        while (checkIndex == 0) {

            while (checkIndex < (CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry))) {
                
                if (memcmp((void *)driver_state.dir_table_buf.table[checkIndex].name, request.name, 8) == 0
                && memcmp((void *)driver_state.dir_table_buf.table[checkIndex].ext, request.ext, 3) == 0) {
                    return 1;
                }

                if (!isFound) {

                    if (!driver_state.dir_table_buf.table[entry].undelete) {
                        isFound = 1;
                    }
                    else {
                        entry++;
                    }
                }
                checkIndex++;
            }

            if (driver_state.fat_table.cluster_map[currentClusterNumber] != FAT32_FAT_END_OF_FILE) {
                read_clusters((void *)&driver_state.dir_table_buf, driver_state.fat_table.cluster_map[currentClusterNumber], 1);
                currentClusterNumber = driver_state.fat_table.cluster_map[currentClusterNumber];
                checkIndex = 0;

                if (!isFound) {
                    entry = 0;
                }
            }
        }

        // check if request is folder
        if (request.buffer_size == 0) {
            struct FAT32DirectoryTable temp_table;

            init_directory_table(&temp_table, request.name, request.parent_cluster_number);

            write_clusters(&temp_table, clusterIndex, 1);

            struct FAT32DirectoryEntry temp_entry = temp_table.table[0];
            temp_entry.cluster_high = (uint16_t)(clusterIndex >> 16);
            temp_entry.cluster_low = (uint16_t)(clusterIndex & 0xFFFF);

            driver_state.dir_table_buf.table[0].user_attribute = UATTR_NOT_EMPTY;

            driver_state.fat_table.cluster_map[clusterIndex] = FAT32_FAT_END_OF_FILE;
            driver_state.dir_table_buf.table[entry] = temp_entry;

            write_clusters((void *)&driver_state.fat_table, FAT_CLUSTER_NUMBER, 1);
            write_clusters((void *)&driver_state.dir_table_buf, currentParentClusterNumber, 1);

            // int empty_entry = -1;
            // for (int i = 0; i < DIRECTORY_ENTRIES_PER_CLUSTER; i++) {
            //     struct FAT32DirectoryEntry* entry = &(driver_state.dir_table_buf.table[i]);
            //     if (entry->name[0] == 0x00 || entry->name[0] == 0xE5) {
            //         // Found an empty entry
            //         empty_entry = i;
            //         break;
            //     }
            // }

            // if (empty_entry == -1) {
            //     // No empty entries found in the directory table
            //     // Handle error here
            // }
        }

        else {
            int sizeTotal = request.buffer_size;
            uint8_t fragmentCtr = 0;
            struct FAT32DirectoryEntry *temp_entry = (void *)&(driver_state.dir_table_buf.table[entry]);
            memcpy(temp_entry->name, request.name, 8);
            memcpy(temp_entry->ext, request.ext, 3);
            temp_entry->cluster_high = (uint16_t)(clusterIndex >> 16);
            temp_entry->cluster_low = (uint16_t)(clusterIndex & 0xFFFF);
            temp_entry->filesize = sizeTotal;
            temp_entry->user_attribute = UATTR_NOT_EMPTY;
            temp_entry->undelete = 1;

            driver_state.dir_table_buf.table[0].user_attribute = UATTR_NOT_EMPTY;

            int prevClustIdx = -1;
            while (sizeTotal > 0) {
                if (prevClustIdx != -1) {
                    driver_state.fat_table.cluster_map[prevClustIdx] = clusterIndex;
                    prevClustIdx--;
                }

                driver_state.fat_table.cluster_map[clusterIndex] = FAT32_FAT_END_OF_FILE;

                write_clusters((void *)&driver_state.fat_table, FAT_CLUSTER_NUMBER, 1);
                write_clusters(&request.buf + fragmentCtr * CLUSTER_SIZE, clusterIndex, 1);
                sizeTotal -= CLUSTER_SIZE;
                fragmentCtr++;

                if (sizeTotal > 0) {
                    prevClustIdx = clusterIndex;

                    // reset clusterIndex by looping
                    clusterIndex = 3;
                    while (driver_state.fat_table.cluster_map[clusterIndex] != 0 && clusterIndex < CLUSTER_MAP_SIZE) {
                        clusterIndex++;
                    }
                }
            }

            write_clusters((void *)&driver_state.dir_table_buf, currentParentClusterNumber, 1);
        }

        return 0;
    }

    else {
        return 2;
    }

    return -1;
}
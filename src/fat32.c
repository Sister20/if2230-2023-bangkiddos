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

void init_directory_table(struct FAT32DirectoryTable *dir_table, char *name, uint32_t parent_dir_cluster) {
    // Set all directory entries to be empty
    for (unsigned int i = 0; i < CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry); i++) {
        memset(dir_table->table[i].name, 0, 8);
        memset(dir_table->table[i].ext, 0, 3);
        dir_table->table[i].cluster_high = 0;
        dir_table->table[i].cluster_low = 0;
        dir_table->table[i].filesize = 0;
    }

    // Initialize the first directory entry with the parent directory
    for (int i = 0; i < strlen(name) && i < 8; i++) {
        dir_table->table[0].name[i] = name[i];
    }
    for (int i = strlen(name); i < 8; i++) {
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

bool is_empty_storage(void) {
    struct ClusterBuffer read = {0};

    read_blocks(&read, BOOT_SECTOR, 1); // Read boot sector from block 0
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

/* Error code: 0 success - 1 not a folder - 2 not found - -1 unknown */
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

    // entry (found requested)
    struct FAT32DirectoryEntry entry = driver_state.dir_table_buf.table[foundIndex];

    // If not a folder
    if (entry.attribute == ATTR_SUBDIRECTORY) { 
        return 1;
    }

    // combines the 16-bit values of entry.cluster_high and entry.cluster_low into a single 32-bit integer value
    int32_t entry_cluster = entry.cluster_high << 16 | entry.cluster_low;

    read_clusters(&request.buf, entry_cluster, 1);

    return 0; // Return success code
}

/* Error code: 0 success - 1 not a file - 2 not enough buffer - 3 not found - -1 unknown */
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

    // entry (found requested)
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

    // loop until eof
    do {
        read_clusters(&request.buf + cluster_count * CLUSTER_SIZE, current_cluster, 1);
        next_cluster = driver_state.fat_table.cluster_map[current_cluster];
        current_cluster = next_cluster;
        cluster_count++;
    } while (next_cluster != FAT32_FAT_END_OF_FILE);

    return 0; // Return success code
}

/* Error code: 0 success - 1 file/folder already exist - 2 invalid parent cluster - -1 unknown */
int8_t write(struct FAT32DriverRequest request) {
    // Read fat and directory table
    read_clusters(&driver_state.fat_table, FAT_CLUSTER_NUMBER, 1);
    read_clusters(&driver_state.dir_table_buf, request.parent_cluster_number, 1);

    // If invalid parent cluster
    if (!isDirectoryValid(request.parent_cluster_number)) {
        return 2;
    }

    // If already exists
    if (isFileOrFolderExists(request.parent_cluster_number, request)) {
        return 1;
    }

    uint32_t allocatedSize = request.buffer_size / CLUSTER_SIZE;

    // If request.buffer_size is 0, make sub directory in parent folder
    if (request.buffer_size % CLUSTER_SIZE != 0) {
        allocatedSize += 1;
    }

    // If not, write until cluster = ceil(request.buffer_size / CLUSTER_SIZE)
    
    uint16_t count = 0;
    int16_t i_before = 0;
    int16_t i_start = -1;

    for (int i = 3; i < 512; i ++) {
        if (!allocatedSize && request.buffer_size != 0) {
            break;
        }
        if (driver_state.fat_table.cluster_map[i] != 0 || driver_state.fat_table.cluster_map[i] == FAT32_FAT_END_OF_FILE) {
            continue;
        }
        
        if (i_start == -1) {
            i_start = i;
        }

        // Check if request is a folder
        if (request.buffer_size == 0) {
            i_before = i;
            struct FAT32DirectoryTable temp;
            init_directory_table(&temp, request.name, request.parent_cluster_number);
            write_clusters(&temp, i, 1);
            break;
        } else {
            write_clusters(&request.buf + count * CLUSTER_SIZE, i, 1);
            request.buf++;
            
            allocatedSize -= 1;

            if (count != 0) {
                driver_state.fat_table.cluster_map[i_before] = i;
            }

            count ++;
            i_before = i;
        }
    }

    driver_state.fat_table.cluster_map[i_before] = FAT32_FAT_END_OF_FILE;

    // Add to directory to parent
    addToDirectory(request.parent_cluster_number, request, i_start);
    write_clusters(&driver_state.fat_table, 1, 1);

    return 0;
    
    return -1; // ??
}

int8_t delete(struct FAT32DriverRequest request) {
    // Search for the entry in the directory table
    int8_t foundIndex = dirtable_linear_search(request.parent_cluster_number, request);
    if (foundIndex == -1) {
        return 1;
    } else if (foundIndex != 0) {

        // Read directory table
        read_clusters(&driver_state.dir_table_buf, request.parent_cluster_number, 1);

        uint32_t theblock = foundIndex;

        // Traverse until we found the end of file
        while (driver_state.fat_table.cluster_map[theblock] != FAT32_FAT_END_OF_FILE) {
            uint32_t oldblock = theblock;
            theblock = driver_state.fat_table.cluster_map[theblock];
            driver_state.fat_table.cluster_map[oldblock] = FAT32_FAT_EMPTY_ENTRY;
        }

        // Delete the end of file
        driver_state.fat_table.cluster_map[theblock] = FAT32_FAT_EMPTY_ENTRY;

        // Count number of entries
        int entry_count = 0;
        for (unsigned int i = 0; i < CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry); i++) {
            if (driver_state.dir_table_buf.table[i].name[0] == 0x00) {
                break;
            } else {
                entry_count++;
            }
        }

        // If there is only 1 entry and it is not root
        if (entry_count == 1) {
            if (memcmp("root", driver_state.dir_table_buf.table[0].name, 8) != 0) {
                // overwrite with fat32 empty
                memset(driver_state.dir_table_buf.table[0].name, 0, 8);
                memset(driver_state.dir_table_buf.table[0].ext, 0, 3);
                driver_state.dir_table_buf.table[0].cluster_high = 0;
                driver_state.dir_table_buf.table[0].cluster_low = 0;
                driver_state.dir_table_buf.table[0].filesize = 0;
            }
        }
        return 0; // Return success code
    } else {
        return 2; // The index is located at index 0 in directorytable
    }
}

int strlen(char *str) {
    // strlen implementation
	int count = 0;

    while (*str != '\0') { // as long as not '\0', iterate
        count ++;
        str ++;
    }

	return count;
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

bool isDirectoryValid(uint32_t parent_cluster_number) {
    /* Check if a directory is valid or not*/
    bool valid = TRUE;
    uint32_t currentCluster = parent_cluster_number;

    // Read fat
    read_clusters(&driver_state.fat_table, FAT_CLUSTER_NUMBER, 1);
    uint16_t counter = 0;

    while (valid) {
        // If at root, valid
        if (currentCluster == ROOT_CLUSTER_NUMBER) {
            break;
        }

        // If exceeded the last valid cluster
        if (driver_state.fat_table.cluster_map[currentCluster] != FAT32_FAT_END_OF_FILE) {
            valid = FALSE;
            break;
        }

        // Read directory table
        read_clusters(&driver_state.dir_table_buf, currentCluster, 1);
        counter ++;

        // If directory is invalid
        if (driver_state.dir_table_buf.table->attribute != ATTR_SUBDIRECTORY || counter >= 512) {
            valid = FALSE;
            break;
        }

        currentCluster = driver_state.dir_table_buf.table->cluster_high << 16 | driver_state.dir_table_buf.table->cluster_low;
    }

    return valid;
}

bool isFileOrFolderExists(uint32_t parent_cluster_number, struct FAT32DriverRequest file_entry) {
    /* Check duplication */
    
    // Read directory table
    read_clusters(&driver_state.dir_table_buf, parent_cluster_number, 1);

    for (int i = 1; i < 64; i++) {
        if (driver_state.dir_table_buf.table[i].attribute == ATTR_SUBDIRECTORY) {
            // Check if file or directory name matches
            if (memcmp(driver_state.dir_table_buf.table[i].name, file_entry.name, 8) == 0 && memcmp(driver_state.dir_table_buf.table[i].ext, file_entry.ext, 3) == 0) {
                return TRUE;
            }
        }
    }

    return FALSE;
}

void addToDirectory(uint32_t parent_cluster_number, struct FAT32DriverRequest entry, int16_t entry_cluster) {
    // Read the current directory
    read_clusters(&driver_state.dir_table_buf, parent_cluster_number, 1); 

    // Loop through all entries
    for(int i = 1; i < 64; i++) { 
        if(driver_state.dir_table_buf.table[i].user_attribute != UATTR_NOT_EMPTY) { 
            if(entry.buffer_size!=0) {
                driver_state.dir_table_buf.table[i].attribute = 0;
            } else {
                driver_state.dir_table_buf.table[i].attribute = ATTR_SUBDIRECTORY;
            }
        
            if(entry.buffer_size != 0) {
                memcpy(driver_state.dir_table_buf.table[i].ext, entry.ext, 3);
            }

            driver_state.dir_table_buf.table[i].access_date = 0;
            
            driver_state.dir_table_buf.table[i].cluster_high = 0; 
            driver_state.dir_table_buf.table[i].cluster_low = entry_cluster; 
            driver_state.dir_table_buf.table[i].create_date = 0; 
            driver_state.dir_table_buf.table[i].create_time = 0; 
            
            memcpy(driver_state.dir_table_buf.table[i].name,entry.name,8); 
            driver_state.dir_table_buf.table[i].filesize = entry.buffer_size; 
            driver_state.dir_table_buf.table[i].modified_date = 0; 
            driver_state.dir_table_buf.table[i].modified_time = 0; 
            driver_state.dir_table_buf.table[i].undelete = 0; 
            driver_state.dir_table_buf.table[i].user_attribute = UATTR_NOT_EMPTY; 
            break; 
        } 
    } 
    write_clusters(&driver_state.dir_table_buf, parent_cluster_number, 1);
}
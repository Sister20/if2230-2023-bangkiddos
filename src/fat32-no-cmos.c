#include <time.h>

#include "lib-header/fat32.h"
#include "lib-header/stdtype.h"
#include "lib-header/stdmem.h"
// #include "lib-header/cmos.h"
#include "lib-header/string.h"

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
    
    dir_table->table[0].attribute = ATTR_SUBDIRECTORY;
    dir_table->table[0].user_attribute = UATTR_NOT_EMPTY;
    dir_table->table[0].cluster_high = (parent_dir_cluster >> 16) & 0xFFFF;
    dir_table->table[0].cluster_low = parent_dir_cluster & 0xFFFF;
    dir_table->table[0].filesize = 0;
    dir_table->table[0].undelete = 0;

    // // cmos
    // struct time t;
    // cmos_read_rtc(&t);
    // dir_table->table[0].create_time = t.hour << 8 | t.minute;
    // dir_table->table[0].create_date = t.year << 9 | t.month << 5 | t.day;
    // dir_table->table[0].access_date = t.year << 9 | t.month << 5 | t.day;

    // using time.h
    struct tm *t;
    time_t now;

    now = time(NULL);
    t = localtime(&now);
    dir_table->table[0].create_time = (t->tm_hour << 8) | t->tm_min;
    dir_table->table[0].create_date = ((t->tm_year + 1900) << 9) | ((t->tm_mon + 1) << 5) | t->tm_mday;
    dir_table->table[0].access_date = dir_table->table[0].create_date;
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
    init_directory_table(&dir_table, "root\0\0\0", 0);
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
    if (entry.attribute != ATTR_SUBDIRECTORY) { 
        return 1;
    }

    // combines the 16-bit values of entry.cluster_high and entry.cluster_low into a single 32-bit integer value
    int32_t entry_cluster = entry.cluster_high << 16 | entry.cluster_low;

    // // cmos
    // struct time t;
    // cmos_read_rtc(&t);
    // entry.access_date = t.year << 9 | t.month << 5 | t.day;

    // using time.h
    struct tm *t;
    time_t now;

    now = time(NULL);
    t = localtime(&now);
    entry.access_date = ((t->tm_year + 1900) << 9) | ((t->tm_mon + 1) << 5) | t->tm_mday;

    read_clusters(request.buf, entry_cluster, 1);

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

    // // cmos
    // struct time t;
    // cmos_read_rtc(&t);
    // entry.access_date = t.year << 9 | t.month << 5 | t.day;
    
    // using time.h
    struct tm *t;
    time_t now;

    now = time(NULL);
    t = localtime(&now);
    entry.access_date = ((t->tm_year + 1900) << 9) | ((t->tm_mon + 1) << 5) | t->tm_mday;

    // loop until eof
    do {
        read_clusters(request.buf + cluster_count * CLUSTER_SIZE, current_cluster, 1);
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

    // 0, 1, 2 defined, so start at 3
    // 0 cluster
    // 1 cluster
    // 2 root
    for (int i = 3; i < 512; i ++) {
        if (driver_state.fat_table.cluster_map[i] != 0 || driver_state.fat_table.cluster_map[i] == FAT32_FAT_END_OF_FILE) {
            continue;
        }
        
        if (i_start == -1) {
            i_start = i;
        }

        // Check if request is a folder
        if (request.buffer_size == 0) {
            i_before = i;
            struct FAT32DirectoryTable temp = {0};
            init_directory_table(&temp, request.name, request.parent_cluster_number);

            // // cmos
            // struct time t;
            // cmos_read_rtc(&t);
            // temp.table->create_time = t.hour << 8 | t.minute;
            // temp.table->create_date = t.year << 9 | t.month << 5 | t.day;
            // temp.table->access_date = t.year << 9 | t.month << 5 | t.day;

            // using time.h
            struct tm *t;
            time_t now;

            now = time(NULL);
            t = localtime(&now);
            temp.table->create_time = (t->tm_hour << 8) | t->tm_min;
            temp.table->create_date = ((t->tm_year + 1900) << 9) | ((t->tm_mon + 1) << 5) | t->tm_mday;
            temp.table->access_date = temp.table->create_date;

            write_clusters(&temp, i, 1);
            break;
        } else {
            struct ClusterBuffer temp[CLUSTER_MAP_SIZE] = {0};
            memcpy(temp, request.buf, request.buffer_size);

            write_clusters(temp->buf + count * CLUSTER_SIZE, i, 1);
            
            allocatedSize -= 1;

            if (count != 0) {
                driver_state.fat_table.cluster_map[i_before] = i;
            }

            count ++;
            i_before = i;
        }

        if (!allocatedSize) {
            break;
        }
    }

    driver_state.fat_table.cluster_map[i_before] = FAT32_FAT_END_OF_FILE;

    // Add to directory
    addToDirectory(request.parent_cluster_number, request, i_start);
    write_clusters(&driver_state.fat_table, 1, 1);

    return 0;
    
    return -1;
}

int8_t delete(struct FAT32DriverRequest request) {
    struct FAT32FileAllocationTable fat = {0};
    read_clusters(&fat, FAT_CLUSTER_NUMBER, 1);

    struct FAT32DirectoryTable dir = {0};
    read_clusters(&dir, request.parent_cluster_number, 1);

    // check if the directory is valid or not, and if it exists or not
    bool isDirValid = isDirectoryValid(request.parent_cluster_number);
    bool isFileExists = isFileOrFolderExists(request.parent_cluster_number, request);

    if (!isDirValid || !isFileExists) {
        return 1; // not found
    }

    int32_t i = 0;

    // if it's a directory
    if (request.buffer_size == 0) {
        int16_t ind = dirtable_linear_search(request.parent_cluster_number, request);
        int32_t cluster_number = driver_state.dir_table_buf.table[ind].cluster_low;

        // check if the directory has files in it 
        if (doesDirHasFiles(cluster_number)) {
            return 2;
        }
        
        i = cluster_number;

        memset(&dir.table[ind], 0, sizeof(struct FAT32DirectoryEntry));
        memset(&fat.cluster_map[cluster_number], 0x0, sizeof(int32_t));
    } else { // if it's a file
        int16_t ind = dirtable_linear_search(request.parent_cluster_number, request);
        int32_t cluster_number = dir.table[ind].cluster_low;
        memset(&dir.table[ind], 0, sizeof(struct FAT32DirectoryEntry));

        i = cluster_number;

        int32_t cur_cluster = cluster_number;
        int32_t curr = fat.cluster_map[cur_cluster];
        // delete all the files until it reaches the end of file
        while (curr != FAT32_FAT_END_OF_FILE) {
            memset(&fat.cluster_map[cur_cluster], 0, sizeof(int32_t));
            cur_cluster = curr;
            curr = fat.cluster_map[cur_cluster];
        }
        // delete the end of file
        memset(&fat.cluster_map[cur_cluster], 0, sizeof(int32_t));
    }
    
    struct ClusterBuffer temp = {0};
    write_clusters(temp.buf, i, 1);
    write_clusters(&fat, FAT_CLUSTER_NUMBER, 1);
    write_clusters(&dir, request.parent_cluster_number, 1);

    return 0; // success

    return -1; // unknown
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
    int8_t found_idx = dirtable_linear_search(parent_cluster_number, file_entry);
    if (found_idx != -1) {
        return TRUE;
    } else {
        return FALSE;
    }
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
            
            driver_state.dir_table_buf.table[i].cluster_high = 0; 
            driver_state.dir_table_buf.table[i].cluster_low = entry_cluster;
            
            // // cmos
            // struct time t;
            // cmos_read_rtc(&t);

            // driver_state.dir_table_buf.table[i].create_time = t.hour << 8 | t.minute;
            // driver_state.dir_table_buf.table[i].create_date = t.year << 9 | t.month << 5 | t.day;
            // driver_state.dir_table_buf.table[i].access_date = t.year << 9 | t.month << 5 | t.day;

            // using time.h
            struct tm *t;
            time_t now;

            now = time(NULL);
            t = localtime(&now);
            driver_state.dir_table_buf.table[0].create_time = (t->tm_hour << 8) | t->tm_min;
            driver_state.dir_table_buf.table[0].create_date = ((t->tm_year + 1900) << 9) | ((t->tm_mon + 1) << 5) | t->tm_mday;
            driver_state.dir_table_buf.table[0].access_date = driver_state.dir_table_buf.table[0].create_date;

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

    return;
}

bool doesDirHasFiles(uint32_t parent_cluster_number) {
    // Read directory table
    struct FAT32DirectoryTable dir_cur = {0};
    read_clusters(&dir_cur, parent_cluster_number, 1);

    // Iterate over each directory entry in the table
    // root is always at 0
    for (int i = 1; i < 64; i++) {
        if (dir_cur.table[i].user_attribute == UATTR_NOT_EMPTY) {
            return TRUE;
        }
    }

    return FALSE;
}

void get_curr_working_dir(uint32_t cur_cluster, struct FAT32DirectoryTable *dir_table) {
    read_clusters(dir_table, cur_cluster, 1);
}

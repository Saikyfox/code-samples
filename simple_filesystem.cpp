#ifndef __PROGTEST__

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <cassert>
#include <functional>

/* Filesystem size: min 8MiB, max 1GiB
 * Filename length: min 1B, max 28B
 * Sector size: 512B
 * Max open files: 8 at a time
 * At most one filesystem mounted at a time.
 * Max file size: < 1GiB
 * Max files in the filesystem: 128
 */

constexpr int FILENAME_LEN_MAX = 28;
constexpr int DIR_ENTRIES_MAX = 128;
constexpr int OPEN_FILES_MAX = 8;
constexpr int SECTOR_SIZE = 512;
constexpr int DEVICE_SIZE_MAX = (1024 * 1024 * 1024);
constexpr int DEVICE_SIZE_MIN = (8 * 1024 * 1024);

struct TFile {
    char m_FileName[FILENAME_LEN_MAX + 1];
    size_t m_FileSize;
};

struct TBlkDev {
    size_t m_Sectors;
    std::function<size_t(size_t, void *, size_t)> m_Read;
    std::function<size_t(size_t, const void *, size_t)> m_Write;
};
#endif /* __PROGTEST__ */

//----------------------------------------------------------------------------------------------------------------------

struct TSUPER_BLOCK{
    size_t total_sectors;
    size_t fat_table_start;
    size_t fat_table_total_sectors;
    size_t bat_table_start;
    size_t bat_table_total_sectors;
};

struct TFAT_ENTRY{
    bool is_valid;
    char filename[FILENAME_LEN_MAX+1];
    size_t starting_sector;
    size_t last_sector;
    size_t file_size;
};

struct TOPEN_FILE_ENTRY{
    int fat_index;
    size_t pos;
    size_t current_sector;
    bool is_valid;
};

struct TSECTOR{
    unsigned char data[SECTOR_SIZE - sizeof(size_t)];
    size_t next_sector;
};

class CBatTable{
public:
    CBatTable() = delete;
    CBatTable(size_t total_sectors) : m_totalSectors(total_sectors), m_last_allocated(0)  {
        m_data = new unsigned char [(total_sectors+7)/8];
        memset(m_data, 0, (total_sectors+7)/8);
        m_size = (total_sectors+7)/8;
    };

    ~CBatTable(){
        delete [] m_data;
    }

    /*
     * Returns: index of allocated sector
     *          SIZE_MAX if FULL
     * */
    size_t allocateSector(){
        size_t start = m_last_allocated;

        for (size_t i = 0; i < m_size; ++i) {

            if(m_data[start] == 0xFF){
                start++;
                start %= m_size;
                continue;
            }

            unsigned char select_bit = 1;
            int j;
            bool found = false;

            for (j = 0; j < 8; ++j) {
                select_bit = (unsigned char) (1 << j);
                auto check = (unsigned char) (m_data[start] & select_bit);

                if(start*8 + j >= m_totalSectors){
//                    printf("\n[DEBUG] Overflow fix triggered!!\n");
                    break;
                }

                if(check == 0) {
                    found = true;
                    break;
                }
            }
            if(start*8 + j >= m_totalSectors) {
                start = 0;
                continue;
            }

            if(!found)
                return SIZE_MAX;
            else{
                m_last_allocated = start;
                m_data[start] = m_data[start] | select_bit;
                return start*8 + j;
            }
        }

        return SIZE_MAX;
    }

    bool allocateSector(size_t sec){
        size_t start = sec/8;
        auto j = sec%8;

        auto select_bit = (unsigned char) (1 << j);
        auto check = (unsigned char) (m_data[start] & select_bit);

        if(check != 0){
            return false;
        }

        m_data[start] = m_data[start] | select_bit;
        return true;
    }

    bool freeSector(size_t sector) {
        unsigned char j = sector % 8;
        size_t start = (sector - j)/8;

        if(start >= m_size)
            return false;

        unsigned char mask = (1 << j);
        unsigned char check = m_data[start] & mask;

        if(check == 0)
            return false;
        else
        {
            mask = ~mask;
            m_data[start] = m_data[start] & mask;
            return true;
        }
    }

    unsigned char * m_data;
    size_t m_totalSectors;
    size_t m_size;
    size_t m_last_allocated;
};

/*
 * Function writes n bytes to disk in continuous way to sectors, starting form start sector
 * Returns: total_sectors written to disk
 * */
size_t write_to_disk(unsigned char * src, size_t n, const TBlkDev & dev, size_t start){
    unsigned char sector_data[SECTOR_SIZE] = {0};
    size_t offset_src = 0;
    int bytes_to_copy = static_cast<int>(n) > SECTOR_SIZE ? SECTOR_SIZE : static_cast<int>(n);
    size_t total_sectors = 0;

    while (n > 0){

        memcpy(sector_data, src + offset_src, bytes_to_copy);


        if(dev.m_Write(start++, sector_data, 1) != 1){
            return 0;
        }
        total_sectors++;

        n -= bytes_to_copy;
        offset_src += bytes_to_copy;
        bytes_to_copy = static_cast<int>(n) > SECTOR_SIZE ? SECTOR_SIZE : static_cast<int>(n);
    }

    return total_sectors;
}

/*
 * Function reads from disk in a continuous way and writes content of n bytes to dst buffer,
 * starting from start sector
 * Returns: true on success
 * */
bool read_from_disk(unsigned char * dst, size_t n, const TBlkDev & dev, size_t start){
    unsigned char sector_data[SECTOR_SIZE];
    size_t offset_dst = 0;
    int bytes_to_copy = static_cast<int>(n) > SECTOR_SIZE ? SECTOR_SIZE : static_cast<int>(n);

    while (n > 0){

        if(dev.m_Read(start++, sector_data, 1) != 1){
            return false;
        }

        memcpy(dst + offset_dst, sector_data, bytes_to_copy);

        n -= bytes_to_copy;
        offset_dst += bytes_to_copy;
        bytes_to_copy = static_cast<int>(n) > SECTOR_SIZE ? SECTOR_SIZE : static_cast<int>(n);
    }

    return true;
}

class CFileSystem {
public:

    CFileSystem(const TBlkDev & dev, size_t total_sec) : disk(dev), m_fat_table(), m_bat_table(total_sec), m_super_block(),
    m_open_files(), m_open_files_total(0) {};

    static bool createFs(const TBlkDev &dev){
        TSUPER_BLOCK super_block{};
        super_block.total_sectors = dev.m_Sectors;


        CBatTable bat_table(super_block.total_sectors);
        TFAT_ENTRY fat_table[DIR_ENTRIES_MAX] = {};


        bat_table.allocateSector(0); // Allocate super block
        // Calculates size of Bat table
        size_t needed = bat_table.m_size * sizeof(unsigned char);
        int bytes_to_copy = needed > SECTOR_SIZE ? SECTOR_SIZE : (int) needed;

        super_block.bat_table_start = bat_table.allocateSector();
        super_block.bat_table_total_sectors = 1;

        needed -= bytes_to_copy;
        bytes_to_copy = needed > SECTOR_SIZE ? SECTOR_SIZE : (int) needed;

        // Allocate Bat table
        while (needed > 0){
            auto sector = bat_table.allocateSector();

            if(sector == SIZE_MAX)
                return false;

            needed -= bytes_to_copy;
            bytes_to_copy = needed > SECTOR_SIZE ? SECTOR_SIZE : (int) needed;
            super_block.bat_table_total_sectors++;
        }

        bat_table.allocateSector();
        // Calculates size of FAT table
        needed = sizeof(TFAT_ENTRY) * DIR_ENTRIES_MAX;
        super_block.fat_table_start = bat_table.allocateSector();
        super_block.fat_table_total_sectors = 1;
        bytes_to_copy = needed > SECTOR_SIZE ? SECTOR_SIZE : (int) needed;
        needed -= bytes_to_copy;

        while (needed > 0){
            auto sector = bat_table.allocateSector();

            if(sector == SIZE_MAX)
                return false;

            needed -= bytes_to_copy;
            bytes_to_copy = needed > SECTOR_SIZE ? SECTOR_SIZE : (int) needed;
            super_block.fat_table_total_sectors++;
        }
        bat_table.allocateSector();

        // Write to disk
        char sector_data[SECTOR_SIZE] = {0};
        memcpy(sector_data, &super_block, sizeof(super_block));

        if(dev.m_Write(0, sector_data, 1) != 1){
            return false;
        }

        if(write_to_disk(bat_table.m_data, bat_table.m_size, dev, super_block.bat_table_start) == 0)
            return false;

        if(write_to_disk(reinterpret_cast<unsigned char *>(fat_table), sizeof(TFAT_ENTRY) * DIR_ENTRIES_MAX, dev, super_block.fat_table_start) == 0)
            return false;

        return true;
    }

    static CFileSystem * mount(const TBlkDev & dev){
        unsigned char sector_data[SECTOR_SIZE] = {};

        TSUPER_BLOCK super_block{};
        dev.m_Read(0, sector_data, 1);
        memcpy(&super_block, sector_data, sizeof(super_block));

        auto * fs_new = new CFileSystem(dev, super_block.total_sectors);
        fs_new->m_super_block = super_block;

        read_from_disk(fs_new->m_bat_table.m_data, fs_new->m_bat_table.m_size, dev, super_block.bat_table_start);

        read_from_disk(reinterpret_cast<unsigned char *>(fs_new->m_fat_table), sizeof(TFAT_ENTRY) * DIR_ENTRIES_MAX, dev, super_block.fat_table_start);


        return fs_new;
    }

    bool umount(){

        // Write super_block
        char setor_data[SECTOR_SIZE] = {};
        memcpy(setor_data, &m_super_block, sizeof(m_super_block));

        if(disk.m_Write(0, setor_data, 1) != 1)
            return false;

        // Write Bat table
        if(!write_to_disk(m_bat_table.m_data, m_bat_table.m_size, disk, m_super_block.bat_table_start))
            return false;

        // Write Fat table
        if(!write_to_disk(reinterpret_cast<unsigned char *>(m_fat_table), sizeof(TFAT_ENTRY) * DIR_ENTRIES_MAX, disk, m_super_block.fat_table_start))
            return false;

        // Close openfiles
        for (int i = 0; i < OPEN_FILES_MAX; ++i) {
            m_open_files[i].is_valid = false;
        }

        return true;
    }

    size_t fileSize(const char *fileName){

        auto file_i = getFile(fileName);
        if(file_i == SIZE_MAX)
            return SIZE_MAX;

        return m_fat_table[file_i].file_size;
    }

    int openFile(const char *fileName, bool writeMode){

        if(m_open_files_total >= OPEN_FILES_MAX)
            return -1;

        auto file_i = getFile(fileName);

        if(writeMode){

            if(file_i != SIZE_MAX){
                if(!delete_file_fat_index(file_i))
                    return -1;
            }

            auto file_new_i = createFile(fileName);

            if(file_new_i == -1)
                return -1;

            if(m_open_files_total < OPEN_FILES_MAX){
                return getOpenEntry(file_new_i);
            }
            return -1;
        }
        else {
            // Read mode
            if(file_i == SIZE_MAX){
                return -1;
            }

            // If file already opened
            for (int i = 0; i < OPEN_FILES_MAX; ++i) {
                if(m_open_files[i].is_valid){
                    if(m_open_files[i].fat_index == static_cast<int>(file_i))
                        return i;
                }
            }

            return getOpenEntry(static_cast<int>(file_i));
        }
    }

    bool closeFile(int fd){

        if(fd < 0 || fd >= OPEN_FILES_MAX)
            return false;

        if(!m_open_files[fd].is_valid)
            return false;

        m_open_files_total--;
        m_open_files[fd].is_valid = false;

        return true;
    }

    size_t readFile(int fd, void *data, size_t len){
        char * data_ptr = (char *) data;

        if(data_ptr == nullptr || fd < 0 || fd >= OPEN_FILES_MAX || len == 0)
            return 0;

        if(!m_open_files[fd].is_valid)
            return 0;

        TOPEN_FILE_ENTRY & file_open_entry = m_open_files[fd];
        TFAT_ENTRY & file_fat_entry = m_fat_table[file_open_entry.fat_index];

        if(!file_fat_entry.is_valid)
            return 0;
        if(file_open_entry.pos == file_fat_entry.file_size)
            return 0;

        TSECTOR sector_data{};

        // Get sector index
        size_t sector_i = file_open_entry.current_sector;

        size_t rem_bytes = len;
        size_t sector_offset = file_open_entry.pos % sizeof(TSECTOR::data);

        // Check if we read all bytes from current sec, jump to next sec
        if(file_open_entry.pos > 0 && sector_offset == 0){
            disk.m_Read(sector_i, &sector_data, 1);
            sector_i = sector_data.next_sector;
        }

        size_t data_offset = 0;
        size_t bytes_to_copy = rem_bytes > sizeof(TSECTOR::data) ? sizeof(TSECTOR::data) : rem_bytes;
        bytes_to_copy = std::min(bytes_to_copy, sizeof(TSECTOR::data) - sector_offset);
        bytes_to_copy = std::min(bytes_to_copy, file_fat_entry.file_size - file_open_entry.pos);


        while (rem_bytes > 0 && sector_i != SIZE_MAX){
            disk.m_Read(sector_i, &sector_data, 1);
            memcpy(data_ptr + data_offset, ((char *) &sector_data) + sector_offset, bytes_to_copy);
            rem_bytes -= bytes_to_copy;
            sector_offset = 0;
            data_offset += bytes_to_copy;
            file_open_entry.pos += bytes_to_copy;
            file_open_entry.current_sector = sector_i;
            bytes_to_copy = rem_bytes > sizeof(TSECTOR::data) ? sizeof(TSECTOR::data) : rem_bytes;
            bytes_to_copy = std::min(bytes_to_copy, file_fat_entry.file_size - file_open_entry.pos);

            sector_i = sector_data.next_sector;
        }
        return len - rem_bytes;
    }

    size_t writeFile(int fd, const void * data, size_t len){
        char * data_ptr = (char *) (data);

        if(fd < 0 || fd >= OPEN_FILES_MAX || data == nullptr || len == 0)
            return 0;
        if(!m_open_files[fd].is_valid)
            return 0;

        TOPEN_FILE_ENTRY & file_open_entry = m_open_files[fd];
        TFAT_ENTRY & file_fat_entry = m_fat_table[file_open_entry.fat_index];

        if(!file_fat_entry.is_valid)
            return 0;

        size_t sector_offset = file_fat_entry.file_size % sizeof(TSECTOR::data);
        size_t data_offset = 0;
        size_t rem_bytes = len;
        size_t sector_i = file_fat_entry.last_sector;
        size_t bytes_to_write = rem_bytes > sizeof(TSECTOR::data) ? sizeof(TSECTOR::data) : rem_bytes;
        bytes_to_write = std::min(sizeof(TSECTOR::data) - sector_offset, bytes_to_write);

        TSECTOR sector_data{};
        if(disk.m_Read(sector_i, &sector_data, 1) != 1){
            return 0;
        }

        // if last sector was full
        if(file_fat_entry.file_size > 0 && sector_offset == 0){
            sector_data.next_sector = m_bat_table.allocateSector();
            if(sector_data.next_sector == SIZE_MAX)
                return 0;
            file_fat_entry.last_sector = sector_data.next_sector;
            disk.m_Write(sector_i, &sector_data, 1);
            sector_i = sector_data.next_sector;
            sector_data.next_sector = SIZE_MAX;
        }

        memcpy( ((char *) &sector_data) + sector_offset, data_ptr + data_offset, bytes_to_write);
        rem_bytes -= bytes_to_write;
        file_fat_entry.file_size += bytes_to_write;
        data_offset += bytes_to_write;

        if(rem_bytes > 0) {
            sector_data.next_sector = m_bat_table.allocateSector();
            if(sector_data.next_sector != SIZE_MAX)
                file_fat_entry.last_sector = sector_data.next_sector;
        }
        if(disk.m_Write(sector_i, &sector_data, 1) != 1)
            return 0;

        if(rem_bytes > 0)
            sector_i = sector_data.next_sector;


        while (rem_bytes > 0 && sector_i != SIZE_MAX){

            bytes_to_write = rem_bytes > sizeof(TSECTOR::data) ? sizeof(TSECTOR::data) : rem_bytes;
            memcpy(&sector_data, data_ptr + data_offset, bytes_to_write);
            rem_bytes -= bytes_to_write;
            data_offset += bytes_to_write;
            file_fat_entry.file_size += bytes_to_write;

            if(rem_bytes > 0){
                sector_data.next_sector = m_bat_table.allocateSector();
                if(sector_data.next_sector != SIZE_MAX)
                    file_fat_entry.last_sector = sector_data.next_sector;
            } else
                sector_data.next_sector = SIZE_MAX;

            disk.m_Write(sector_i, &sector_data, 1);
            sector_i = sector_data.next_sector;
        }

        return len - rem_bytes;
    }

    bool deleteFile(const char *fileName){
        auto file_i = getFile(fileName);
        if(file_i == SIZE_MAX)
            return false;

        return delete_file_fat_index(file_i);
    }

    bool findFirst(TFile &file){

        for (int i = 0; i < DIR_ENTRIES_MAX; ++i) {
            if(m_fat_table[i].is_valid){
                for (int j = 0; j < FILENAME_LEN_MAX; ++j) {
                    file.m_FileName[j] = m_fat_table[i].filename[j];
                }
                file.m_FileSize = m_fat_table[i].file_size;
                return true;
            }
        }

        return false;
    }

    bool findNext(TFile &file){

        int i;
        for (i = 0; i < DIR_ENTRIES_MAX; ++i) {
            if(m_fat_table[i].is_valid){
                if(filename_cmp(m_fat_table[i].filename, file.m_FileName)){
                    i++;
                    break;
                }
            }
        }

        for (int j = i; j < DIR_ENTRIES_MAX; ++j) {
            if(m_fat_table[j].is_valid){
                for (int k = 0; k < FILENAME_LEN_MAX; ++k) {
                    file.m_FileName[k] = m_fat_table[j].filename[k];
                }
                file.m_FileSize = m_fat_table[j].file_size;
                return true;
            }
        }

        return false;
    }

private:

    /*
     * Function delete file from fat table
     * - validates if fat index is valid,
     * - Invalidates fat entry if success
     * */
    bool delete_file_fat_index(size_t fat_index){
        if(fat_index >= DIR_ENTRIES_MAX)
            return false;

        if(!m_fat_table[fat_index].is_valid)
            return false;

        // Close file
        for (int i = 0; i < OPEN_FILES_MAX; ++i) {
            if(m_open_files[i].is_valid){
                if(m_open_files[i].fat_index == static_cast<int>(fat_index)){
                    m_open_files[i].is_valid = false;
                    m_open_files_total--;
                }
            }
        }

        size_t head = m_fat_table[fat_index].starting_sector;
        TSECTOR sector_data{};
        if(disk.m_Read(head, &sector_data, 1) != 1)
            return false;

        m_fat_table[fat_index].is_valid = false;

        while (true){

            if(!m_bat_table.freeSector(head))
                break;

            head = sector_data.next_sector;

            if(head == SIZE_MAX)
                break;

            disk.m_Read(head, &sector_data, 1);
        }



        return true;
    }

    static bool filename_cmp(const char * name_a, const char * name_b){
        bool result = true;

        for (int i = 0; i < FILENAME_LEN_MAX + 1; ++i) {
            if(name_a[i] == 0 || name_b[i] == 0){
                result = name_a[i] == name_b[i];
                break;
            }
            if(name_a[i] != name_b[i]){
                result = false;
                break;
            }
        }

        return result;
    }

    /*
     * Function take filename and search if such file exists in fat table
     * Returns:  index to Fat table
     *           SIZE_MAX if no file exists
     * */
    size_t getFile(const char * fileName){

        if(fileName == nullptr)
            return SIZE_MAX;

        for (int i = 0; i < DIR_ENTRIES_MAX; ++i) {
            if(m_fat_table[i].is_valid){
                if(filename_cmp(m_fat_table[i].filename, fileName)){
                    return i;
                }
            }
        }
        return SIZE_MAX;
    }

    /*
     * Creates new file in Fat table.
     * Returns:  index to fat table
     *           -1 on fail
     * */
    int createFile(const char * fileName){
        if(fileName == nullptr)
            return -1;

        bool found = false;
        int file_i = 0;
        for (int i = 0; i < DIR_ENTRIES_MAX; ++i) {
            if(!m_fat_table[i].is_valid){
                file_i = i;
                found = true;
                break;
            }
        }

        if(!found)
            return -1;

        auto & file_entry = m_fat_table[file_i];

        file_entry.file_size = 0;
        file_entry.starting_sector = m_bat_table.allocateSector();

        if(file_entry.starting_sector == SIZE_MAX)
            return -1;

        TSECTOR sector_data{};
        sector_data.next_sector = SIZE_MAX;
        if(disk.m_Write(file_entry.starting_sector, &sector_data, 1) != 1)
            return false;

        file_entry.last_sector = file_entry.starting_sector;
        file_entry.is_valid = true;

        for (int i = 0; i < FILENAME_LEN_MAX; ++i) {
            file_entry.filename[i] = fileName[i];

            if(fileName[i] == 0)
                break;
        }
        file_entry.filename[FILENAME_LEN_MAX] = 0;

        return file_i;
    }


    /*
     * Creates an open file entry, not validating FAT index
     * Returns:  index to open_files
     *           -1 on fail
     * */
    int getOpenEntry(int fat_index){
        for (int i = 0; i < OPEN_FILES_MAX; ++i) {
            if(!m_open_files[i].is_valid){
                m_open_files_total++;

                m_open_files[i].fat_index = fat_index;
                m_open_files[i].pos = 0;
                m_open_files[i].is_valid = true;
                m_open_files[i].current_sector = m_fat_table[fat_index].starting_sector;
                return i;
            }
        }
        return -1;
    }
public:

    TBlkDev disk;
    TFAT_ENTRY m_fat_table[DIR_ENTRIES_MAX];
    CBatTable m_bat_table;
    TSUPER_BLOCK m_super_block;
    TOPEN_FILE_ENTRY m_open_files[OPEN_FILES_MAX];
    unsigned int m_open_files_total;
};

#ifndef __PROGTEST__

#include "simple_test.cpp"

#endif /* __PROGTEST__ */

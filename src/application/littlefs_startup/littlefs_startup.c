/*
 * littlefs_startup.c
 *
 * Created: may 2024
 *  Author: PI
 */ 

#define LFS_NO_DEBUG

#include "littlefs_startup.h"
#include "lfs.h"
#include "uart_printf.h"
#include "debug.h"

//#include "driver_w25qxx.h"


// TODO: use MSP to get w25qxx handle
extern w25qxx_handle_t w25q128_handle;

// variables used by the filesystem
lfs_t       lfs_global;
lfs_file_t  lfs_global_file;
lfs_dir_t   lfs_global_dir;


// create module with the following functions
// TODO: glue w25q128_handle into the struct lfs_config *c????

// Read a region in a block. Negative error codes are propagated to the user.
static int user_provided_block_device_read(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size)
{
    /* extract block-size from lfs_config *c */
    uint32_t blockSize = c->block_size;
    uint32_t addr = blockSize * block + off;
    /* extract w25q128_handle from const struct lfs_config *c */
    w25qxx_handle_t *flashChipDriverHandle = (w25qxx_handle_t*)c->context;
    uint8_t res = w25qxx_read(flashChipDriverHandle, addr, (uint8_t *)buffer, size);
    return res;
}

 // Program a region in a block. The block must have previously been erased. Negative error codes are propagated to the user.
 // May return LFS_ERR_CORRUPT if the block should be considered bad.
static int user_provided_block_device_prog(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size)
{
    /* extract block-size from lfs_config *c */
    uint32_t blockSize = c->block_size;
    uint32_t addr = blockSize * block + off;
    /* extract w25q128_handle from const struct lfs_config *c */
    w25qxx_handle_t *flashChipDriverHandle = (w25qxx_handle_t*)c->context;
    uint8_t res = w25qxx_page_program(flashChipDriverHandle, addr, (uint8_t *)buffer, size);
    return res;
}

 // Erase a block. A block must be erased before being programmed. The state of an erased block is undefined. Negative error codes
 // are propagated to the user. May return LFS_ERR_CORRUPT if the block should be considered bad.
static int user_provided_block_device_erase(const struct lfs_config *c, lfs_block_t block)
{
    /* extract block-size from lfs_config *c */
    uint32_t blockSize = c->block_size;
    uint32_t addr = blockSize * block;
    /* extract w25q128_handle from const struct lfs_config *c */
    w25qxx_handle_t *flashChipDriverHandle = (w25qxx_handle_t*)c->context;
    uint8_t res = w25qxx_sector_erase_4k(flashChipDriverHandle, addr);
    return (-1 * res);
}

// Sync the state of the underlying block device. Negative error codes are propagated to the user.
static int user_provided_block_device_sync(const struct lfs_config *c)
{
    return 0;
}


// configuration of the filesystem is provided by this struct
const struct lfs_config cfg = {

     // flash memory chip driver handle
     .context = (void*)&w25q128_handle,

     // block device operations
     .read  = user_provided_block_device_read,
     .prog  = user_provided_block_device_prog,
     .erase = user_provided_block_device_erase,
     .sync  = user_provided_block_device_sync,

     // block device configuration
     .read_size = 1,            // can read 1?? or 256 byte
     .prog_size = 256,          // can program 256 byte
     .block_size = 4096,        // erasable block size is 4096 byte
     .block_count = 4096,        // 128 Mbit chip has 4096 sectors of 4096 byte or 256 blocks of 64KB 
     .cache_size = 256,         // should be multiple of read_size/prog_size
     .lookahead_size = 256,     // should be multiple of read_size/prog_size
     .block_cycles = 500,
 };


int8_t Littlefs_Init()
{
    // mount the filesystem
    int8_t err = lfs_mount(&lfs_global, &cfg);

    // reformat if we can't mount the filesystem, this should only happen on the first boot
    if (err)
    {
        err = lfs_format(&lfs_global, &cfg);           
        err = lfs_mount(&lfs_global, &cfg);
    }

    return err;
}


int8_t Littlefs_Startup()
{    
     int err = Littlefs_Init();
     if (err)
     {
         TRACE_PRINTF("\r\n-------------------LittleFS mount FAIL! Error = %d\r\n", err);
     }
     else
     {
         TRACE_PRINTF("\r\n+++++++++++++++++++LittleFS mount OK!\r\n");
     }
     
 #if 0    
     // read current count
     uint32_t boot_count = 0;
     
     lfs_file_open(&lfs_global, &lfs_global_file, "boots.txt", LFS_O_RDWR | LFS_O_CREAT);
     lfs_file_read(&lfs_global, &lfs_global_file, &boot_count, sizeof(boot_count));

     // update boot count
     boot_count += 1;
     lfs_file_rewind(&lfs_global, &lfs_global_file);
     lfs_file_write(&lfs_global, &lfs_global_file, &boot_count, sizeof(boot_count));

     // remember the storage is not updated until the file is closed successfully
     lfs_file_close(&lfs_global, &lfs_global_file);

    // print the boot count
    uart_print("littlefs: boot_count: %d\r\n", boot_count);
    //printf("littlefs: boot_count: %d\n", boot_count);
    
    //Create test directory:   
    err = lfs_mkdir(&lfs_global, "/test"); // Returns a negative error code on failure.
    if (err < 0)
    {
        printf("littlefs: lfs_mkdir(/test) ERROR!!: %d\n", err);
    }
    err = lfs_mkdir(&lfs_global, "foobar"); // Returns a negative error code on failure.
    if (err < 0)
    {
        printf("littlefs: lfs_mkdir(foobar) ERROR!!: %d\n", err);
    }
#endif    
    
    // release any resources we were using
    lfs_unmount(&lfs_global);
    
    return 0;
}


// Those below are not implemented properly. See fs_port_custom_littlefs.c for reference
void Littlefs_FileOpen(void *file, const char *path, const unsigned int mode)
{
    int err = 0;
    lfs_file_t *lfsFile = (lfs_file_t *)file;
    err = lfs_file_open(&lfs_global, lfsFile, path, (enum lfs_open_flags)mode); // TODO: maybe it is necessary to convert uint_t mode to enum lfs_open_flags in specific manner??
    if(err)
    {
        file = NULL;
    }
}

void Littlefs_FileClose(void *file)
{
    lfs_file_t *lfsFile = (lfs_file_t *)file;
    (void)lfs_file_close(&lfs_global, lfsFile);
}

uint8_t Littlefs_FileExists(const char *path)
{
    int res = 0;
    res = lfs_file_open(&lfs_global, &lfs_global_file, path, LFS_O_EXCL); // open file with LFS_O_EXCL flag: if file exists lfs_file_open() fails
    return (0 == res) ? 0 : 1;
}

uint8_t Littlefs_FileDelete(const char *path)
{
    int res = 0;
    res = lfs_remove(&lfs_global, path); // Returns a negative error code on failure.
    return (res >= 0) ? 0 : 1;
}

uint8_t Littlefs_FileRename(const char *oldPath, const char *newPath)
{
    int res = 0;
    res = lfs_rename(&lfs_global, oldPath, newPath); // Returns a negative error code on failure.
    return (res >= 0) ? 0 : 1;
}

uint8_t Littlefs_FileWrite(void *file, void *data, size_t length)
{
    lfs_ssize_t res = -1;
    res = lfs_file_write(&lfs_global, (lfs_file_t *)file, (const void *)data, (lfs_size_t)length); // Returns the number of bytes written, or a negative error code on failure.
    return (res >= 0) ? 0 : 1;
}

uint8_t Littlefs_FileRead(void *file, void *data, size_t size, size_t *length)
{
    lfs_ssize_t res = -1;
    res = lfs_file_read(&lfs_global, (lfs_file_t *)file, (void *)data, (lfs_size_t)size);// Returns the number of bytes read, or a negative error code on failure.
    if (res >= 0)
    {
        *length = res;
        return 0;
    }
    else
    {
        *length = 0;
    }
    
    return 1;
}

uint8_t Littlefs_FileSizeGet(const char *path, uint32_t *size)
{
    lfs_soff_t res = -1;
    void *file = NULL;

    Littlefs_FileOpen(file, path, LFS_O_RDONLY);            // Open file
    if(NULL == file)
    {
        return 1;
    }

    res = lfs_file_size(&lfs_global, (lfs_file_t *)file);   // Returns the size of the file, or a negative error code on failure.(&lfs_global, (lfs_file_t *)file, (lfs_soff_t)offset, (int32_t)origin);
    
    Littlefs_FileClose(file);                               // Close file

    if (res >= 0)
    {
        *size = res;
    }
    else
    {
        *size = 0;
    }

    return (res >= 0) ? 0 : 1;
}

uint8_t Littlefs_FileSeek(void *file, int32_t offset, int32_t origin)
{
    lfs_soff_t res = -1;
    res = lfs_file_seek(&lfs_global, (lfs_file_t *)file, (lfs_soff_t)offset, (int32_t)origin);
    return (res >= 0) ? 0 : 1;
}

uint8_t Littlefs_FileStatGet(const char *path, void *fileStat)
{
    int res = -1; 
    // Fills out the info structure, based on the specified file or directory. Find info about a file or directory.
    res = lfs_stat(&lfs_global, path, (struct lfs_info *)fileStat); // Returns a negative error code on failure.
    return (res >= 0) ? 0 : 1;
}

//===================================================[DIRS]======================================

uint8_t Littlefs_DirExists(const char *path)
{
    int res = -1;
    lfs_dir_t *dir = NULL;
    res = lfs_dir_open(&lfs_global, dir, path); // Open a directory. Returns a negative error code on failure.
    return (res == 0) ? 1 : 0;
}

uint8_t Littlefs_DirCreate(const char *path)
{
    lfs_ssize_t res = -1;
    res = lfs_mkdir(&lfs_global, path); // Returns a negative error code on failure
    return (res >= 0) ? 0 : 1;
}

uint8_t Littlefs_DirRemove(const char *path)
{
    lfs_ssize_t res = -1;
    res = lfs_remove(&lfs_global, path); // If removing a directory, the directory must be empty. Returns a negative error code on failure.
    return (res >= 0) ? 0 : 1;
}

uint8_t Littlefs_DirRead(void *dir, void *dirEntry)    ///??? params
{
    lfs_ssize_t res = -1;
    // Fills out the info structure, based on the specified file or directory.
    // Returns a positive value on success, 0 at the end of directory, or a negative error code on failure.
    res = lfs_dir_read(&lfs_global, (lfs_dir_t *)dir, (struct lfs_info *)dirEntry);
    return (res >= 0) ? 0 : 1;
}

uint8_t Littlefs_DirOpen(const char *path, void *dir)    ///??? params
{
    lfs_ssize_t res = -1;
    res = lfs_dir_open(&lfs_global, &lfs_global_dir, path); // Returns a negative error code on failure.
    return res;
}

void Littlefs_DirClose(void *dir)
{
    lfs_ssize_t res = -1;
    res = lfs_dir_close(&lfs_global, (lfs_dir_t *)dir); // Returns a negative error code on failure.
    (void)res;
    return;
}

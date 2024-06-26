/*
 * fs_port_littlefs.c
 *
 * Created: may 2024
 *  Author: PI
 */

#include "fs_port.h"
#include "fs_port_custom.h"

#include "error.h"

// module print level
//#define TRACE_LEVEL     TRACE_LEVEL_VERBOSE
//#define TRACE_LEVEL     TRACE_LEVEL_DEBUG   

#include "debug.h"


//File system objects
static lfs_t        fs;
static lfs_file_t   fileTable[FS_MAX_FILES];
static lfs_dir_t    dirTable[FS_MAX_DIRS];

// flash chip configuration
extern const struct lfs_config cfg;

//Mutex that protects critical sections
static OsMutex fsMutex;


/**
 * @brief File system initialization
 * @return Error code
 **/
error_t fsInit(void)
{
    TRACE_DEBUG("..........fsInit(void)..........\r\n");
    int res = 0;

    //Clear file system objects
    osMemset(fileTable, 0, sizeof(fileTable));
    osMemset(dirTable, 0, sizeof(dirTable));

    //Create a mutex to protect critical sections
    if(!osCreateMutex(&fsMutex))
    {
        //Failed to create mutex
        return ERROR_OUT_OF_RESOURCES;
    }

    //Mount file system
    res = lfs_mount(&fs, &cfg);

    // reformat if we can't mount the filesystem, this should only happen on the first boot
    if (LFS_ERR_OK != res)
    {
        res = lfs_format(&fs, &cfg);           
        res = lfs_mount(&fs, &cfg);
    }

    //Failed to mount file system?
    if(LFS_ERR_OK != res)
    {
        //Clean up side effects
        osDeleteMutex(&fsMutex);
        //Report an error
        return ERROR_FAILURE;
    }
    
#if 0     
    //Create test directory:
    res = lfs_mkdir(&fs, "/test"); // Returns a negative error code on failure.
    if (res < 0)
    {
        TRACE_ERROR("ftp_server_startup: lfs_mkdir(/test) ERROR!!: %d\r\n", res);
    } 
    res = lfs_mkdir(&fs, "foobar"); // Returns a negative error code on failure.
    if (res < 0)
    {
        TRACE_ERROR("ftp_server_startup: lfs_mkdir(foobar) ERROR!!: %d\r\n", res);
    }
#endif   
    
    //osDelayTask(1000);
    
    // read current count
    uint32_t boot_count = 0;
    lfs_file_t file = { 0 };
     
    lfs_file_open(&fs, &file, "boots.txt", LFS_O_RDWR | LFS_O_CREAT);
    lfs_file_read(&fs, &file, &boot_count, sizeof(boot_count));

    // update boot count
    boot_count += 1;
    lfs_file_rewind(&fs, &file);
    lfs_file_write(&fs, &file, &boot_count, sizeof(boot_count));

    // remember the storage is not updated until the file is closed successfully
    lfs_file_close(&fs, &file);

    // print the boot count
    TRACE_DEBUG("littlefs: boot_count: %d\r\n", boot_count);
    
    struct lfs_fsinfo fsInfo = { 0 };
    // Fills out the fsinfo structure based on the filesystem found on-disk. Returns a negative error code on failure.
    res = lfs_fs_stat(&fs, &fsInfo);
    if (0 == res)
    {
        TRACE_DEBUG("FILESYSTEM: Total logical blocks=%d; block size=%d(Bytes); total size=%d(Bytes)\r\n", fsInfo.block_count, fsInfo.block_size, fsInfo.block_count * fsInfo.block_size);
    }
    // Returns the number of allocated blocks, or a negative error code on failure.
    res =  lfs_fs_size(&fs);
    if (res >= 0)
    {
        TRACE_DEBUG("FILESYSTEM: Allocated logical blocks=%d; block size=%d(Bytes); total allocated size=%d(Bytes)\r\n", res, fsInfo.block_size, res * fsInfo.block_size);
    }
    
    
    //Successful processing
    return NO_ERROR;
}

//=======================================[FILE MANIPULATIONS]=============================================

/**
 * @brief Check whether a file exists
 * @param[in] path NULL-terminated string specifying the filename
 * @return The function returns TRUE if the file exists. Otherwise FALSE is returned
 **/
bool_t fsFileExists(const char_t *path)
{
    TRACE_VERBOSE("..........fsFileExists(%s)..........\r\n", path);
    int32_t res = 0;
    struct lfs_info info = { 0 };

    //Make sure the pathname is valid
    if(NULL == path)
        return FALSE;
    
#ifdef USE_MUTEX
    //Enter critical section
    osAcquireMutex(&fsMutex);
#endif
    
    //Check whether the file exists
    res = lfs_stat(&fs, (const char *)path, &info); // Returns a negative error code on failure.

#ifdef USE_MUTEX    
    //Leave critical section
    osReleaseMutex(&fsMutex);
#endif

    //Any error to report?
    if(LFS_ERR_OK != res)
        return FALSE;

    //Valid file?
    if(LFS_TYPE_REG == info.type)
      return TRUE;
    
    return FALSE;
}


/**
 * @brief Open the specified file for reading or writing
 * @param[in] path NULL-terminated string specifying the filename
 * @param[in] mode Type of access permitted (FS_FILE_MODE_READ,
 *   FS_FILE_MODE_WRITE or FS_FILE_MODE_CREATE)
 * @return File handle
 **/
FsFile *fsOpenFile(const char_t *path, uint_t mode)
{
    TRACE_VERBOSE("..........fsOpenFile(%s, mode=%x)..........\r\n", path, mode);
    uint_t i = 0;
    uint_t flags = 0;
    int32_t res = 0;

   //File pointer
   lfs_file_t *file = NULL;

   //Make sure the pathname is valid
   if(NULL == path)
      return NULL;

#ifdef USE_MUTEX
    //Enter critical section
    osAcquireMutex(&fsMutex);
#endif

   //Loop through the file objects
   for(i = 0; i < FS_MAX_FILES; i++)
   {
      //Unused file object found?
      if(0 == fileTable[i].id) // TODO: is this a proper way to find empty slot for file handle??? .ctz.size
      {
         //Default access mode
         flags = 0;

         //Check access mode
         if(mode & FS_FILE_MODE_READ)
            flags |= LFS_O_RDONLY;

         if(mode & FS_FILE_MODE_WRITE)
            flags |= LFS_O_WRONLY;

         if(mode & FS_FILE_MODE_CREATE)
            flags |= LFS_O_CREAT;

         if(mode & FS_FILE_MODE_TRUNC)
            flags |= LFS_O_TRUNC;

         //Open the specified file
         res = lfs_file_open(&fs, (lfs_file_t*)&fileTable[i], (const char *)path, flags);

         //Check status code
         if(LFS_ERR_OK == res)
            file = &fileTable[i];

         //Stop immediately
         break;
      }
   }

#ifdef USE_MUTEX    
    //Leave critical section
    osReleaseMutex(&fsMutex);
#endif
   
   //Return a handle to the file
   return file;
}


/**
 * @brief Close a file
 * @param[in] file Handle that identifies the file to be closed
 **/
void fsCloseFile(FsFile *file)
{
    TRACE_VERBOSE("..........fsCloseFile(file=%p)..........\r\n", file);
    //Make sure the file pointer is valid
    if(NULL == file)
       return;
   
#ifdef USE_MUTEX
    //Enter critical section
    osAcquireMutex(&fsMutex);
#endif

    //Close the specified file
    (void)lfs_file_close(&fs, (lfs_file_t*)file);
    
    //TODO: ?? Mark the corresponding entry as free file->... = NULL???
    osMemset(file, 0, sizeof(lfs_file_t));

#ifdef USE_MUTEX    
    //Leave critical section
    osReleaseMutex(&fsMutex);
#endif
}


/**
 * @brief Retrieve the size of the specified file
 * @param[in] path NULL-terminated string specifying the filename
 * @param[out] size Size of the file in bytes
 * @return Error code
 **/
error_t fsGetFileSize(const char_t *path, uint32_t *size)
{
    TRACE_VERBOSE("..........fsGetFileSize(%s, )..........\r\n", path);
    int32_t res = 0;
    struct lfs_info fileInfo = { 0 };
    //void *file = NULL;
    
    //Check parameters
    if((NULL == path) || (NULL == size))
        return ERROR_INVALID_PARAMETER;

#ifdef USE_MUTEX
    //Enter critical section
    osAcquireMutex(&fsMutex);
#endif
    
    // Fills out the info structure, based on the specified file or directory. Returns a negative error code on failure.
    res = lfs_stat(&fs, path, &fileInfo);
    //res = lfs_file_size(&fs, file);   // Returns the size of the file, or a negative error code on failure.
    
#ifdef USE_MUTEX    
    //Leave critical section
    osReleaseMutex(&fsMutex);
#endif
    
    if (res < 0)
        return ERROR_FAILURE;              // error
    
    if (LFS_TYPE_DIR == fileInfo.type)
        return ERROR_FAILURE;              // this is not a file but a directory
    
    *size = fileInfo.size;
    
    TRACE_VERBOSE("..........fsGetFileSize(%s,...) = %d..........\r\n", path, *size);
    
    return NO_ERROR;
}


/**
 * @brief Retrieve the attributes of the specified file
 * @param[in] path NULL-terminated string specifying the filename
 * @param[out] fileStat File attributes
 * @return Error code
 **/
error_t fsGetFileStat(const char_t *path, FsFileStat *fileStat)
{
    TRACE_VERBOSE("..........fsGetFileStat(%s,...)..........\r\n", path);
    int32_t res = 0;
    struct lfs_info info = { 0 };

    //Check parameters
    if((NULL == path) || (NULL == fileStat))
        return ERROR_INVALID_PARAMETER;

#ifdef USE_MUTEX
    //Enter critical section
    osAcquireMutex(&fsMutex);
#endif

    //Retrieve information about the specified file
    res = lfs_stat(&fs, path, &info);
    
#ifdef USE_MUTEX    
    //Leave critical section
    osReleaseMutex(&fsMutex);
#endif

   //Any error to report?
   if(LFS_ERR_OK != res)
      return ERROR_FAILURE;

   //Clear file attributes
   osMemset(fileStat, 0, sizeof(FsFileStat));

   //File attributes
   if(LFS_TYPE_REG == info.type)
   {
       fileStat->attributes = 0x01;
   }
   else if (LFS_TYPE_DIR == info.type)
   {
       fileStat->attributes = 0x10; // FOR DIRECTORIES!!??
   }    
   //File size
   fileStat->size = info.size;

   //TODO: Set time of last modification here
   //TODO: Make sure the date is valid


   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Rename the specified file
 * @param[in] oldPath NULL-terminated string specifying the pathname of the file to be renamed
 * @param[in] newPath NULL-terminated string specifying the new filename
 * @return Error code
 **/
error_t fsRenameFile(const char_t *oldPath, const char_t *newPath)
{
    TRACE_VERBOSE("..........fsRenameFile(old=%s, new=%s)..........\r\n", oldPath, newPath);
#ifdef LFS_READONLY
    //Read-only configuration
    return ERROR_READ_ONLY_ACCESS;
#else
    int32_t res = 0;

    //Check parameters
    if((NULL == oldPath) || (NULL == newPath))
        return ERROR_INVALID_PARAMETER;

#ifdef USE_MUTEX
    //Enter critical section
    osAcquireMutex(&fsMutex);
#endif

    //Rename the specified file
    res = lfs_rename(&fs, oldPath, newPath);

#ifdef USE_MUTEX    
    //Leave critical section
    osReleaseMutex(&fsMutex);
#endif

    //Any error to report?
    if(LFS_ERR_OK != res)
        return ERROR_FAILURE;

    //Successful processing
    return NO_ERROR;
#endif
}


/**
 * @brief Delete a file
 * @param[in] path NULL-terminated string specifying the filename
 * @return Error code
 **/
error_t fsDeleteFile(const char_t *path)
{
    TRACE_VERBOSE("..........fsDeleteFile(%s)..........\r\n", path);
#ifdef LFS_READONLY
    //Read-only configuration
    return ERROR_READ_ONLY_ACCESS;
#else
    int32_t res = 0;

    //Check parameters
    if(NULL == path)
        return ERROR_INVALID_PARAMETER;

#ifdef USE_MUTEX
    //Enter critical section
    osAcquireMutex(&fsMutex);
#endif

    //Delete the specified file
    res = lfs_remove(&fs, path);

#ifdef USE_MUTEX    
    //Leave critical section
    osReleaseMutex(&fsMutex);
#endif

    //Any error to report?
    if(LFS_ERR_OK != res)
        return ERROR_FAILURE;

    //Successful processing
    return NO_ERROR;
#endif
}


/**
 * @brief Move to specified position in file
 * @param[in] file Handle that identifies the file
 * @param[in] offset Number of bytes to move from origin
 * @param[in] origin Position used as reference for the offset (FS_SEEK_SET,
 *   FS_SEEK_CUR or FS_SEEK_END)
 * @return Error code
 **/
error_t fsSeekFile(FsFile *file, int_t offset, uint_t origin)
{
    TRACE_VERBOSE("..........fsSeekFile(file=%p, offset=%d, origin=%d).........\r\n", file, offset, origin);
    int32_t res = 0;
    int32_t whence = FS_SEEK_SET;

    //Check parameters
    if(NULL == file)
        return ERROR_INVALID_PARAMETER;

#ifdef USE_MUTEX
    //Enter critical section
    osAcquireMutex(&fsMutex);
#endif

    //Find whence flag
    if(FS_SEEK_CUR == origin)
    {
        whence = LFS_SEEK_CUR;
    }
    else if(FS_SEEK_END == origin)
    {
        whence = LFS_SEEK_END;
    }
    else
    {
        //The offset is absolute
    }

    //Move read/write pointer
    // Returns the new position of the file, or a negative error code on failure.
    res = lfs_file_seek(&fs, (lfs_file_t*)file, (lfs_soff_t)offset, whence);

#ifdef USE_MUTEX    
    //Leave critical section
    osReleaseMutex(&fsMutex);
#endif
    
    TRACE_VERBOSE("..........lfs_file_seek(file=%p, offset=%d, whence=%d)=%d.........\r\n", file, offset, whence, res);
    
    //Any error to report?
    if(res < 0)
        return ERROR_FAILURE;

    //Successful processing
    return NO_ERROR;
}


/**
 * @brief Write data to the specified file
 * @param[in] file Handle that identifies the file to be written
 * @param[in] data Pointer to a buffer containing the data to be written
 * @param[in] length Number of data bytes to write
 * @return Error code
 **/
error_t fsWriteFile(FsFile *file, void *data, size_t length)
{
    TRACE_VERBOSE("..........fsWriteFile(file=%p, ... , length=%d).........\r\n", file, length);
#ifdef LFS_READONLY
    //Read-only configuration
    return ERROR_READ_ONLY_ACCESS;
#else
    
    //Check parameters
    if(NULL == file)
        return ERROR_INVALID_PARAMETER;
   
    int32_t res = 0;
    
#ifdef USE_MUTEX
    //Enter critical section
    osAcquireMutex(&fsMutex);
#endif

    //Write data
    // Returns the number of bytes written, or a negative error code on failure.
    res = lfs_file_write(&fs, (lfs_file_t *)file, (const void *)data, (lfs_size_t)length);

#ifdef USE_MUTEX    
    //Leave critical section
    osReleaseMutex(&fsMutex);
#endif

    //Any error to report?
    if(res < 0)
        return ERROR_FAILURE;

    //Sanity check
    if(res != length)
        return ERROR_FAILURE;

    //Successful processing
    return NO_ERROR;
#endif
}
//==OK

/**
 * @brief Read data from the specified file
 * @param[in] file Handle that identifies the file to be read
 * @param[in] data Pointer to the buffer where to copy the data
 * @param[in] size Size of the buffer, in bytes
 * @param[out] length Number of data bytes that have been read
 * @return Error code
 **/
error_t fsReadFile(FsFile *file, void *data, size_t size, size_t *length)
{
    TRACE_VERBOSE("..........fsReadFile(file=%p, ..., size=%d, ...).........\r\n", file, size);
    int32_t res = 0;

    //Check parameters
    if((NULL == file) || (NULL == length))
        return ERROR_INVALID_PARAMETER;

    //No data has been read yet
    *length = 0;

#ifdef USE_MUTEX
    //Enter critical section
    osAcquireMutex(&fsMutex);
#endif

    //Read data
    // Returns the number of bytes read, or a negative error code on failure.
    res = lfs_file_read(&fs, (lfs_file_t *)file, (void *)data, (lfs_size_t)size);

#ifdef USE_MUTEX    
    //Leave critical section
    osReleaseMutex(&fsMutex);
#endif

    //Any error to report?
    if(res < 0)
        return ERROR_FAILURE;

    //EOF?
    if(0 == res)
        return ERROR_END_OF_FILE;

    //Total number of data that have been read
    *length = res;
    TRACE_VERBOSE("..........fsReadFile(file=%p, ..., size=%d, length=%d).........\r\n", file, size, *length);
    
    //Successful processing
    return NO_ERROR;
}

//=======================================[DIRECTORY MANIPULATIONS]=============================================

/**
 * @brief Check whether a directory exists
 * @param[in] path NULL-terminated string specifying the directory path
 * @return The function returns TRUE if the directory exists. Otherwise FALSE is returned
 **/
bool_t fsDirExists(const char_t *path)
{
    TRACE_VERBOSE("..........fsDirExists(%s).........\r\n", path);
    int32_t res = 0;
    struct lfs_info info = { 0 };

    //Make sure the pathname is valid
    if(NULL == path)
        return FALSE;

    //Root directory?
    if(!osStrcmp(path, "/"))
        return TRUE;

#ifdef USE_MUTEX
    //Enter critical section
    osAcquireMutex(&fsMutex);
#endif

    //Check whether the file exists
    res = lfs_stat(&fs, (const char *)path, &info); // Returns a negative error code on failure.

#ifdef USE_MUTEX    
    //Leave critical section
    osReleaseMutex(&fsMutex);
#endif

    //Any error to report?
    if(LFS_ERR_OK != res)
        return FALSE;

    //Valid directory?
    if(LFS_TYPE_DIR == info.type)
      return TRUE;
    
    return FALSE;
}


/**
 * @brief Create a directory
 * @param[in] path NULL-terminated string specifying the directory path
 * @return Error code
 **/
error_t fsCreateDir(const char_t *path)
{
    TRACE_VERBOSE("..........fsCreateDir(%s).........\r\n", path);
#ifdef LFS_READONLY
    //Read-only configuration
    return ERROR_READ_ONLY_ACCESS;
#else
    
    //Check parameters
    if(NULL == path)
        return ERROR_INVALID_PARAMETER;
   
    int32_t res = 0;
    
#ifdef USE_MUTEX
    //Enter critical section
    osAcquireMutex(&fsMutex);
#endif

    //Create a new directory
    res = lfs_mkdir(&fs, path); // Returns a negative error code on failure

#ifdef USE_MUTEX    
    //Leave critical section
    osReleaseMutex(&fsMutex);
#endif

    //Any error to report?
    if(LFS_ERR_OK != res)
        return ERROR_FAILURE;

    //Successful processing
    return NO_ERROR;
#endif
}

/**
 * @brief Remove a directory
 * @param[in] path NULL-terminated string specifying the directory path
 * @return Error code
 **/
error_t fsRemoveDir(const char_t *path)
{
    TRACE_VERBOSE("..........fsRemoveDir(%s).........\r\n", path);
#ifdef LFS_READONLY
    //Read-only configuration
    return ERROR_READ_ONLY_ACCESS;
#else
    
    //Check parameters
    if(NULL == path)
        return ERROR_INVALID_PARAMETER;
   
    int32_t res = 0;
    
#ifdef USE_MUTEX
    //Enter critical section
    osAcquireMutex(&fsMutex);
#endif

    //Remove the specified directory
    res = lfs_remove(&fs, path); // If removing a directory, the directory must be empty. Returns a negative error code on failure.

#ifdef USE_MUTEX    
    //Leave critical section
    osReleaseMutex(&fsMutex);
#endif

    //Any error to report?
    if(LFS_ERR_OK != res)
        return ERROR_FAILURE;

    //Successful processing
    return NO_ERROR;
#endif
}


/**
 * @brief Open a directory stream
 * @param[in] path NULL-terminated string specifying the directory path
 * @return Directory handle
 **/
FsDir *fsOpenDir(const char_t *path)
{
    TRACE_VERBOSE("..........fsOpenDir(%s).........\r\n", path);
    //Directory pointer
    FsDir *dir = NULL;

    //Check parameters
    if(NULL == path)
        return ERROR_INVALID_PARAMETER;
   
    int32_t res = 0;
    uint32_t i = 0;

#ifdef USE_MUTEX
    //Enter critical section
    osAcquireMutex(&fsMutex);
#endif

   //Loop through the directory objects
    for(i = 0; i < FS_MAX_DIRS; i++)
    {
        //Unused directory object found?
        if(0 == dirTable[i].id) // TODO: is this a proper way to find out..?
        {
            //Open the specified directory
            res = lfs_dir_open(&fs, (lfs_dir_t*)&dirTable[i], path); // Returns a negative error code on failure.

            //Check status code
            if(LFS_ERR_OK == res)
                dir = &dirTable[i];

            //Stop immediately
            break;
        }
    }

#ifdef USE_MUTEX    
    //Leave critical section
    osReleaseMutex(&fsMutex);
#endif
    
    //Return a handle to the directory
    return dir;
}


/**
 * @brief Read an entry from the specified directory stream
 * @param[in] dir Handle that identifies the directory
 * @param[out] dirEntry Pointer to a directory entry
 * @return Error code
 **/
error_t fsReadDir(FsDir *dir, FsDirEntry *dirEntry)
{  
    TRACE_VERBOSE("..........fsReadDir(dir=%p, ...).........\r\n", dir);
    int32_t res = 0;
    struct lfs_info lfsDirEntry = { 0 };
    size_t n = 0;

    //Make sure the directory pointer is valid
    if(NULL == dir)
        return ERROR_INVALID_PARAMETER;

#ifdef USE_MUTEX
    //Enter critical section
    osAcquireMutex(&fsMutex);
#endif

    //Read the specified directory
    // Fills out the info structure, based on the specified file or directory. Returns a positive value on success, 0 at the end of directory, or a negative error code on failure.
    res = lfs_dir_read(&fs, (lfs_dir_t *)dir, &lfsDirEntry);

#ifdef USE_MUTEX    
    //Leave critical section
    osReleaseMutex(&fsMutex);
#endif

    //Any error to report?
    if(res < 0)
        return ERROR_FAILURE;
    else if(0 == res)   //End of the directory stream? if(fno.fname[0] == '\0')
        return ERROR_END_OF_STREAM;

    //File attributes
    
    //dirEntry->attributes = lfsDirEntry.type;                // Determine if this is a file(0x01) or dir(0x02)
    
    if(LFS_TYPE_REG == lfsDirEntry.type)
    {
        dirEntry->attributes = 0x01;        // for files??
    }
    else if (LFS_TYPE_DIR == lfsDirEntry.type)
    {
        dirEntry->attributes = 0x10;        // FOR DIRECTORIES!!??
    }
    TRACE_VERBOSE("..........fsReadDir: lfsDirEntry.type = %d.........\r\n", lfsDirEntry.type);
    TRACE_VERBOSE("..........fsReadDir: fileStat.attributes = %d.........\r\n", dirEntry->attributes);
    
    //File size
    dirEntry->size = lfsDirEntry.size;
    //Copy the time of last modification: not available in littleFS
    //Make sure the date is valid: no need

    //Retrieve the length of the file name
    n = osStrlen(lfsDirEntry.name);
    //Limit the number of characters to be copied
    n = MIN(n, FS_MAX_NAME_LEN);

    //Copy file name
    osStrncpy(dirEntry->name, lfsDirEntry.name, n);
    //Properly terminate the string with a NULL character
    dirEntry->name[n] = '\0';

    //Successful processing
    return NO_ERROR;
}


/**
 * @brief Close a directory stream
 * @param[in] dir Handle that identifies the directory to be closed
 **/
void fsCloseDir(FsDir *dir)
{
    TRACE_VERBOSE("..........fsCloseDir(dir=%p).........\r\n", dir);
    //Make sure the directory pointer is valid
    if(NULL == dir)
        return;
    
#ifdef USE_MUTEX
    //Enter critical section
    osAcquireMutex(&fsMutex);
#endif

    //Close the specified directory
    lfs_dir_close(&fs, (lfs_dir_t *)dir);   // Returns a negative error code on failure.

    //Mark the corresponding entry as free
    //((lfs_dir_t *)dir)->id = 0; 
    osMemset(dir, 0, sizeof(lfs_dir_t));    // TODO: is this a proper way to ..?

#ifdef USE_MUTEX    
    //Leave critical section
    osReleaseMutex(&fsMutex);
#endif    
}

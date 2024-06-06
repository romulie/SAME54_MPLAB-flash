/*
 * fs_port_custom.h
 *
 * Created: may 2024
 *  Author: PI
 */ 

#ifndef FS_PORT_CUSTOM_H_
#define FS_PORT_CUSTOM_H_

//Dependencies
#include "os_port.h"
#include "lfs.h"

// if use mutex to protect critical sections
#define USE_MUTEX

//Number of files that can be opened simultaneously
#ifndef FS_MAX_FILES
    #define FS_MAX_FILES 3
#elif (FS_MAX_FILES < 1)
    #error FS_MAX_FILES parameter is not valid
#endif

//Number of directories that can be opened simultaneously
#ifndef FS_MAX_DIRS
    #define FS_MAX_DIRS 3
#elif (FS_MAX_DIRS < 1)
    #error FS_MAX_DIRS parameter is not valid
#endif

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief File descriptor
 **/
typedef void FsFile;

/**
 * @brief Directory descriptor
 **/
typedef void FsDir;


//File system abstraction layer
error_t fsInit(void);

bool_t fsFileExists(const char_t *path);
error_t fsGetFileSize(const char_t *path, uint32_t *size);
error_t fsGetFileStat(const char_t *path, FsFileStat *fileStat);
error_t fsRenameFile(const char_t *oldPath, const char_t *newPath);
error_t fsDeleteFile(const char_t *path);

FsFile *fsOpenFile(const char_t *path, uint_t mode);
error_t fsSeekFile(FsFile *file, int_t offset, uint_t origin);
error_t fsWriteFile(FsFile *file, void *data, size_t length);
error_t fsReadFile(FsFile *file, void *data, size_t size, size_t *length);
void fsCloseFile(FsFile *file);

bool_t fsDirExists(const char_t *path);
error_t fsCreateDir(const char_t *path);
error_t fsRemoveDir(const char_t *path);

FsDir *fsOpenDir(const char_t *path);
error_t fsReadDir(FsDir *dir, FsDirEntry *dirEntry);
void fsCloseDir(FsDir *dir);


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* FS_PORT_CUSTOM_H_ */
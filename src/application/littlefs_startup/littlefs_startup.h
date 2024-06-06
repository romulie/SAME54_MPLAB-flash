/*
 * littlefs_startup.h
 *
 * Created: may 2024
 *  Author: PI
 */ 


#ifndef LITTLEFS_STARTUP_H_
#define LITTLEFS_STARTUP_H_

#include "driver_w25qxx.h"
#include "lfs.h"

int8_t Littlefs_Init();
int8_t Littlefs_Startup();

void Littlefs_FileOpen(void *file, const char *path, const unsigned int mode);
void Littlefs_FileClose(void *file);
uint8_t Littlefs_FileExists(const char *path);
uint8_t Littlefs_FileDelete(const char *path);
uint8_t Littlefs_FileRename(const char *oldPath, const char *newPath);

uint8_t Littlefs_FileRead(void *file, void *data, size_t size, size_t *length);
uint8_t Littlefs_FileWrite(void *file, void *data, size_t length);

uint8_t Littlefs_FileSizeGet(const char *path, uint32_t *size);
uint8_t Littlefs_FileSeek(void *file, int32_t offset, int32_t origin);
uint8_t Littlefs_FileStatGet(const char *path, void *fileStat);


uint8_t Littlefs_DirExists(const char *path);

uint8_t Littlefs_DirCreate(const char *path);
uint8_t Littlefs_DirRemove(const char *path);

uint8_t Littlefs_DirRead(void *dir, void *dirEntry);    ///??? params

uint8_t Littlefs_DirOpen(const char *path, void *dir);  ///??? params
void Littlefs_DirClose(void *dir);

#endif /* LITTLEFS_STARTUP_H_ */
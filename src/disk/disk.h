#ifndef _DISK_H
#define _DISK_H

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../util.h"
#include "worldctrl.h"

#ifdef _WIN32
#define PATH_SEP	 '\\'
#define PATH_SEP_STR "\\"
#include <io.h>
#include <windows.h>
#define mkdir(a) mkdir(a)
/* not implemented in mingw */
#define MAP_FAILED NULL
#define PROT_READ  0x1
#define PROT_WRITE 0x2
#define O_RDWR	   _O_RDWR
#define O_CREAT	   _O_CREAT
#define open	   _open
#define close	   _close
void *mmap(void *addr, size_t length, int prot, int flags, int fd,
		   off_t offset);
int	  munmap(void *addr, size_t length);
/* Unused */
#define MAP_SHARED 0
#else
#define PATH_SEP	 '/'
#define PATH_SEP_STR "/"
#define _GNU_SOURCE
#include <linux/falloc.h>
#include <linux/fs.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <sys/statvfs.h>
#include <sys/types.h>
#include <unistd.h>
#define mkdir(a) mkdir(a, 0755)
#endif

#define _1K (1024)
#define _1M (_1K * _1K)
#define _1G (_1M * _1K)

extern char path_separator[2];

extern char *user_path;
extern char *player_data_folder_path;
extern char *player_sprite_file_path;
extern char *world_folder_path;
extern char *world_control_path;
extern char *world_data_path;

extern WorldControl *world_control;

size_t check_disk_space(const char *path);
int	   file_exists(const char *path);
void   mkdir_r(const char *path);
void   disk_init();

#endif // _DISK_H

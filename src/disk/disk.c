#include "disk.h"

#include "../log/log.h"

char path_separator[2];

#define _1K (1024)
#define _1M (_1K * _1K)
#define _1G (_1M * _1K)

char		 *user_path		   = NULL;
WorldControl *world_control	   = NULL;
int			  world_control_fd = -1;

const char *game_folder			   = ".sandsaga" PATH_SEP_STR;
const char *world_folder		   = "worlds" PATH_SEP_STR "Test" PATH_SEP_STR;
const char *world_sparse_folder	   = "sparse" PATH_SEP_STR;
const char *world_control_filename = "control";

char *world_folder_path	 = NULL;
char *world_control_path = NULL;

static size_t check_disk_space(const char *path) {
#ifdef _WIN32
	ULARGE_INTEGER freeBytesAvailable, totalBytes, freeBytes;
	if (GetDiskFreeSpaceExA(path, &freeBytesAvailable, &totalBytes,
							&freeBytes)) {
		return freeBytesAvailable.QuadPart;
	}
	logerr("Failed to query free space: (%lu) %s\n", GetLastError(),
		   strerror(GetLastError()));
	errno = GetLastError();
	return 0;
#else
	struct statvfs stat;
	if (statvfs(path, &stat) == 0) {
		return stat.f_bavail * stat.f_frsize;
	} else {
		logerr("Failed to query free space: (%lu) %s\n", errno,
			   strerror(errno));
		return 0;
	}
#endif
}

static void mkdir_r(const char *path) {
	char *p = strdup(path);

	for (char *s = strchr(p, PATH_SEP); s != NULL;
		 s		 = strchr(s + 1, PATH_SEP)) {
		if (s == p)
			continue;

		*s = '\0';
		if (access(p, F_OK) != 0)
			mkdir(p);
		*s = PATH_SEP;
	}

	free(p);
}

static void disk_deinit() {
	if (world_control && world_control != MAP_FAILED)
		munmap(world_control, sizeof(WorldControl));

	if (world_control_fd)
		close(world_control_fd);

	if (world_control_path)
		free(world_control_path);

	if (user_path)
		free(user_path);
}

void disk_init() {
	atexit(disk_deinit);

	path_separator[0] = PATH_SEP;
	path_separator[1] = '\0';

	char *home_folder;
#ifdef _WIN32
	home_folder = getenv("USERPROFILE");
#else
	home_folder = getenv("HOME");
#endif

	user_path =
		calloc(strlen(home_folder) + 1 + strlen(game_folder) + 1, sizeof(char));

	strcpy(user_path, home_folder);
	strcat(user_path, path_separator);
	strcat(user_path, game_folder);

	world_folder_path =
		calloc(strlen(user_path) + strlen(world_folder) + 1, sizeof(char));
	strcpy(world_folder_path, user_path);
	strcat(world_folder_path, world_folder);

	/* Create world folder if it doesn't exist */
	mkdir_r(world_folder_path);

	/* Check if we have enough space */
	const size_t free_space = check_disk_space(world_folder_path);
	if (free_space < _1G) {
		logerr("Disk small or running out of space (<1GB).\nFree space: "
			   "%zu MB\n",
			   free_space / _1M);
		exit(1);
	} else {
		loginfo("Free space: %zu bytes\n", free_space);
	}

	world_control_path =
		calloc(strlen(world_folder_path) + strlen(world_control_filename) + 1,
			   sizeof(char));
	strcpy(world_control_path, world_folder_path);
	strcat(world_control_path, world_control_filename);

	world_control_fd = open(world_control_path, O_RDWR | O_CREAT, 0644);
	if (world_control_fd < 0) {
		logerr("Failed to open world control file: %s\n", strerror(errno));
		exit(1);
	}

	if (ftruncate(world_control_fd, sizeof(WorldControl)) != 0) {
		logerr("Failed to set world control file size: %s\n", strerror(errno));
		exit(1);
	}

	/* Map control file to memory */
	world_control = mmap(NULL, sizeof(WorldControl), PROT_READ | PROT_WRITE,
						 MAP_SHARED, world_control_fd, 0);

	if (world_control == MAP_FAILED) {
		logerr("Failed to map world control file: %s\n", strerror(errno));
		exit(1);
	}

	if (!world_control->version) {
		/* Initialize world control file */
		world_control->version = SAVEFILE_VERSION;
		world_control->seed	   = rand();
	} else if (world_control->version != SAVEFILE_VERSION) {
		logerr("World control file version mismatch. Expected: %d Got: %d\n",
			   SAVEFILE_VERSION, world_control->version);
		exit(1);
	}
}

#ifdef _WIN32
/* Windows implementation for missing functions */

typedef struct {
	HANDLE hFile;		 // File handle
	HANDLE hMapping;	 // Mapping handle
	void  *mappedMemory; // Pointer to mapped memory
} MMapHandle;

static MMapHandle *world_control_file_winh = NULL;

void *mmap(void *addr, size_t length, int prot, int flags, int fd,
		   off_t offset) {
	HANDLE file_handle = (HANDLE)_get_osfhandle(fd);
	if (file_handle == INVALID_HANDLE_VALUE) {
		logerr("Invalid file handle: %d\n", fd);
		return MAP_FAILED;
	}

	DWORD protect = 0;
	if (prot & PROT_WRITE) {
		protect = PAGE_READWRITE;
	} else if (prot & PROT_READ) {
		protect = PAGE_READONLY;
	}

	DWORD offset_high = (DWORD)(offset >> 32);
	DWORD offset_low  = (DWORD)(offset & 0xFFFFFFFF);
	DWORD size_high	  = (DWORD)((offset + length) >> 32);
	DWORD size_low	  = (DWORD)((offset + length) & 0xFFFFFFFF);

	HANDLE mapping = CreateFileMapping(file_handle, NULL, protect, size_high,
									   size_low, NULL);
	if (!mapping) {
		logerr("CreateFileMapping failed: %lu\n", GetLastError());
		return MAP_FAILED;
	}

	DWORD access = 0;
	if (prot & PROT_WRITE) {
		access = FILE_MAP_WRITE;
	} else if (prot & PROT_READ) {
		access = FILE_MAP_READ;
	}

	void *map = MapViewOfFile(mapping, access, offset_high, offset_low, length);
	if (!map) {
		CloseHandle(mapping);
		logerr("MapViewOfFile failed: %lu\n", GetLastError());
		return MAP_FAILED;
	}

	return map;
}

int munmap(void *addr, size_t length) {
	if (!UnmapViewOfFile(addr)) {
		logerr("UnmapViewOfFile failed: %lu\n", GetLastError());
		return -1;
	}
	return 0;
}
#endif
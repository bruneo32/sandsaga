file(GLOB disk_src
	"disk.h"
	"disk.c"
	"worldctrl.h"
	"worldctrl.c"
	"resources.h"
	"resources.c"
	)

add_library(disk ${disk_src})
target_link_libraries(disk PRIVATE logger)

target_link_libraries(${PROJECT_NAME} disk)

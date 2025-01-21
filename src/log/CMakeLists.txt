file(GLOB log_src
	"log.h"
	"log.c"
	)

add_library(logger ${log_src})

target_link_libraries(${PROJECT_NAME} logger)

file(GLOB ui_src
	"ui.h"
	"ui.c"
	)

add_library(UI ${ui_src})
target_link_libraries(UI PRIVATE SDL2)

target_link_libraries(${PROJECT_NAME} UI)

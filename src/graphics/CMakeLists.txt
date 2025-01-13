file(GLOB graphics_src
	"graphics.h"
	"graphics.c"
	"color.h"
	"font/font.h"
	"font/font.c"
	)

add_library(graphics ${graphics_src})
target_link_libraries(graphics PRIVATE SDL2)

target_link_libraries(${PROJECT_NAME} graphics)

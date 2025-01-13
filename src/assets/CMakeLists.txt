file(GLOB assets_src
	"assets.h"
	"assets.c"
	"res/*.h"
	)

add_library(assets ${assets_src})

# Link required libs
target_link_libraries(assets PRIVATE SDL2_image)

target_link_libraries(${PROJECT_NAME} assets)

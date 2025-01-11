file(GLOB engine_src
	"bonerig.h"
	"bonerig.c"
	"gameobjects.h"
	"engine.h"
	"engine.c"
	"entities.h"
	"entities.c"
	"noise.h"
	"noise.c"
	)

add_library(engine ${engine_src})

# Link box2d to engine
target_link_libraries(engine PRIVATE physics)

target_link_libraries(${PROJECT_NAME} engine)

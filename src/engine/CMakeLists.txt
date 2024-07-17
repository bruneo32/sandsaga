file(GLOB engine_src
	"engine.h"
	"entities.h"
	"entities.c"
	"gameobjects.h"
	"noise.h"
	"noise.c"
	)
add_library(engine ${engine_src})

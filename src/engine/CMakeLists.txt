file(GLOB engine_src
	"engine.h"
	"gameobjects.h"
	"noise.h"
	"noise.c"
	)
add_library(engine ${engine_src})

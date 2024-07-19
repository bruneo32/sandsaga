file(GLOB engine_src
	"gameobjects.h"
	"engine.h"
	"engine.c"
	"entities.h"
	"entities.c"
	"noise.h"
	"noise.c"
	)
add_library(engine ${engine_src})

file(GLOB assets_src
	"assets.h"
	"assets.c"
	)
add_library(assets ${assets_src})

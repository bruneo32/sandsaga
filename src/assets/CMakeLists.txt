file(GLOB assets_src
	"assets.h"
	"assets.c"
	"res/*.h"
	)
add_library(assets ${assets_src})

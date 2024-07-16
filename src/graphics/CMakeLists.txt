file(GLOB graphics_src
	"graphics.h"
	"graphics.c"
	"color.h"
	)
add_library(graphics ${graphics_src})

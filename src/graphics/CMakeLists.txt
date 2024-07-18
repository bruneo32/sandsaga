file(GLOB graphics_src
	"graphics.h"
	"graphics.c"
	"color.h"
	"font/font.h"
	"font/font.c"
	)
add_library(graphics ${graphics_src})

# Add box2d CMake modules

if (NOT DEFINED BOX2D_INCLUDE_DIRS)
	set(BOX2D_INCLUDE_DIRS "/usr/include/box2d")
endif()

set(BOX2D_LIBRARIES "-lbox2d")

include_directories(
	${BOX2D_INCLUDE_DIRS}
	)

link_libraries(
	${BOX2D_LIBRARIES}
	)

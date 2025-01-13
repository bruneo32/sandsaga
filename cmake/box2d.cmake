# Add box2d CMake modules

include_directories( "${BOX2D_PATH}/include/box2d" )

target_link_directories(${PROJECT_NAME} PRIVATE
	"${BOX2D_PATH}/lib"
)

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
	# Windows libs are DLL, not static
	target_link_libraries(${PROJECT_NAME}
		-Wl,-Bdynamic
		box2d
		)
else()
	target_link_libraries(${PROJECT_NAME}
		-Wl,-Bstatic
		box2d
		-Wl,-Bdynamic
		)
endif()

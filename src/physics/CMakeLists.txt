file(GLOB physics_src
	"physics.h"
	"box2d_wrapper.cpp"
	"meshgen.cpp"
	"RamerDouglasPeucker.hpp"
	"RamerDouglasPeucker.cpp"
	)

# Use C++ compiler
set_source_files_properties(${physics_src} PROPERTIES LANGUAGE CXX)

add_library(physics STATIC ${physics_src})

# Link required libs
target_link_libraries(physics PRIVATE
	graphics
	box2d
	)

# Set C++ standard
target_compile_features(physics PRIVATE cxx_std_17)

# Link to project
target_link_libraries(${PROJECT_NAME} physics)

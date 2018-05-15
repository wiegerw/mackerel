project(core)

file(GLOB SOURCES "source/*.cpp" "source/*.c")
add_library(core ${SOURCES})

#add_subdirectory(test)

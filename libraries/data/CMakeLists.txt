project(data)

file(GLOB SOURCES "source/*.cpp")
add_library(data ${SOURCES})

project(process)

file(GLOB SOURCES "source/*.cpp")
add_library(process ${SOURCES})

project(lps)

file(GLOB SOURCES "source/*.cpp")
add_library(lps ${SOURCES})

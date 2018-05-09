project(atermpp)

file(GLOB SOURCES "source/*.cpp")
add_library(atermpp ${SOURCES})

add_subdirectory(test)

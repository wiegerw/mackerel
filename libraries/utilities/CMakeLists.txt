project(utilities)

file(GLOB SOURCES "source/*.cpp")
add_library(utilities ${SOURCES})

#add_subdirectory(test)

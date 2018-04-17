project(data)

file(GLOB SOURCES "source/*.cpp" "source/detail/rewrite/*.cpp")
add_library(data ${SOURCES})

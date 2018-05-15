project(process)

file(GLOB SOURCES "source/*.cpp")
add_library(process ${SOURCES})
target_link_libraries(data)

#add_subdirectory(test)

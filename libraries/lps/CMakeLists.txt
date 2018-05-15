project(lps)

file(GLOB SOURCES "source/*.cpp")
add_library(lps ${SOURCES})
target_link_libraries(lps data)

#add_subdirectory(test)

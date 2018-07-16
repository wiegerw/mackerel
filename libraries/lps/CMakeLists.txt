project(lps)

file(GLOB SOURCES "source/*.cpp")
add_library(lps ${SOURCES})
target_link_libraries(lps core data dparser)

#add_subdirectory(test)

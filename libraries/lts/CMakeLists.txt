project(lts)

file(GLOB SOURCES "source/*.cpp" "source/*.c")
add_library(lts ${SOURCES})
target_link_libraries(lts data lps)

#add_subdirectory(test)

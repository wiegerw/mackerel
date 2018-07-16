project(mcrl3explore)

add_executable(mcrl3explore mcrl3explore.cpp)
target_link_libraries(mcrl3explore atermpp core data dparser lps lts process utilities)
install(TARGETS mcrl3explore DESTINATION bin)

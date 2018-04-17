project(mcrl3transform)

add_executable(mcrl3transform mcrl3transform.cpp)
target_link_libraries(mcrl3transform atermpp core data dparser lps process utilities)
install(TARGETS mcrl3transform DESTINATION bin)

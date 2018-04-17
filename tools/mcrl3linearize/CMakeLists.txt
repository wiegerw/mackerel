project(mcrl3linearize)

add_executable(mcrl3linearize mcrl3linearize.cpp)
target_link_libraries(mcrl3linearize atermpp core data dparser lps process utilities)
